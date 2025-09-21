#include "pch.h"
#include "CWndMain.h"


void CWndLrc::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
		m_Lrc.LrcSetCurrentLine(App->GetPlayer().GetCurrLrcIdx());
		break;
	case PlayEvt::Play:
	{
		ComPtr<Lyric::CLyric> pLyric;
		App->GetPlayer().GetLrc(pLyric.RefOf());
		m_Lrc.SetLyric(pLyric.Get());
	}
	break;
	}
}

LRESULT CWndLrc::OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCHITTEST:
	{
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ScreenToClient(hWnd, &pt);
		const auto cxPadded = eck::DaGetSystemMetrics(SM_CXPADDEDBORDER, GetDpiValue());
		const auto cxFrame = eck::DaGetSystemMetrics(SM_CXFRAME, GetDpiValue()) + cxPadded;
		const auto cyFrame = eck::DaGetSystemMetrics(SM_CYFRAME, GetDpiValue()) + cxPadded;
		const MARGINS m{ cxFrame,cyFrame,cxFrame,cyFrame };
		auto lResult = eck::MsgOnNcHitTest(pt, m, GetClientWidth(), GetClientHeight());
		if (lResult == HTCAPTION)
		{
			lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);
			if (!GetCurrNcHitElem() || GetCurrNcHitElem() == &m_Lrc)
				lResult = HTCAPTION;
		}
		return lResult;
	}
	break;
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	{
		ECK_DUILOCKWND;
		if (m_bShowBk)
			break;
		m_bShowBk = TRUE;
		m_AnFade.Start(0.0f, 1.0f, m_bAnFade);
		m_bAnFade = TRUE;
		SetTimer(hWnd, IDT_LRC_MOUSELEAVE, TE_LRC_MOUSELEAVE, nullptr);
		WakeRenderThread();
	}
	break;
	case WM_SIZE:
	{
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);
		const auto cxLyt = m_Layout.LoGetSize().cx;
		m_Layout.Arrange(
			int((GetClientWidthLog() - cxLyt) / 2),
			(int)CxyLrcPadding,
			(int)cxLyt, (int)CxyLrcBtn);
		m_Lrc.SetRect({
			CxyLrcPadding,
			float(m_Layout.LoGetPos().y + m_Layout.LoGetSize().cy + CxyLrcPadding),
			GetClientWidthLog() - CxyLrcPadding,
			GetClientHeightLog() - CxyLrcPadding });
		return lResult;
	}
	break;
	case WM_TIMER:
	{
		switch (wParam)
		{
		case IDT_LRC_MOUSELEAVE:
		{
			if (GetCapture() == hWnd)
				return 0;
			RECT rc;
			GetWindowRect(hWnd, &rc);
			POINT pt;
			GetCursorPos(&pt);
			if (!PtInRect(&rc, pt))
			{
				ECK_DUILOCKWND;
				m_bShowBk = FALSE;
				KillTimer(hWnd, IDT_LRC_MOUSELEAVE);
				m_AnFade.Start(1.0f, 0.0f, m_bAnFade);
				m_bAnFade = TRUE;
				WakeRenderThread();
			}
		}
		return 0;
		}
	}
	break;
	case WM_SHOWWINDOW:
	{
		if (m_bInitShow)
			SetTimer(hWnd, IDT_LRC_MOUSELEAVE, TE_LRC_MOUSELEAVE, nullptr);
	}
	break;
	case WM_CREATE:
	{
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);

		App->GetPlayer().GetSignal().Connect(this, &CWndLrc::OnPlayEvent);
		RegisterTimeLine(this);

		m_pVioletTheme = new CVioletTheme{};
		m_pVioletTheme->Init(GetDeviceContext(), GetStdTheme());

		constexpr MARGINS Mar{ .cxRightWidth = (int)CxyLrcPadding };
		m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTPrev.SetBitmap(App->GetMainWindow().RealizeImage(GImg::PrevSolid));
		m_BTPrev.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTPrev, Mar, eck::LF_FIX);

		m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTPlay.SetBitmap(App->GetMainWindow().RealizeImage(GImg::TriangleSolid));
		m_BTPlay.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTPlay, Mar, eck::LF_FIX);

		m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 2, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTNext.SetBitmap(App->GetMainWindow().RealizeImage(GImg::NextSolid));
		m_BTNext.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTNext, Mar, eck::LF_FIX);

		m_BTLock.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 3, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTLock.SetBitmap(App->GetMainWindow().RealizeImage(GImg::LockSolid));
		m_BTLock.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTLock, Mar, eck::LF_FIX);

		m_BTClose.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 4, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTClose.SetBitmap(App->GetMainWindow().RealizeImage(GImg::CrossSolid));
		m_BTClose.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTClose, {}, eck::LF_FIX);

		m_Lrc.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, 0, 0, nullptr, this);
		ComPtr<IDWriteTextFormat> pTfLrc;
		auto& FontFactory = App->GetFontFactory();;
		FontFactory.NewFont(pTfLrc.RefOfClear(),
			eck::Align::Near, eck::Align::Near, 30, 700);
		pTfLrc->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		m_Lrc.SetTextFormat(pTfLrc.Get());
		FontFactory.NewFont(pTfLrc.RefOfClear(),
			eck::Align::Near, eck::Align::Near, 20, 500);
		pTfLrc->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		m_Lrc.SetTextFormatTrans(pTfLrc.Get());
		m_Lrc.LrcSetEmptyText(L"VioletModel - VC++/Win32"sv);

		return lResult;
	}
	break;
	case WM_DESTROY:
	{
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);
		SafeRelease(m_pVioletTheme);
		return lResult;
	}
	break;
	}
	return __super::OnMsg(hWnd, uMsg, wParam, lParam);
}

LRESULT CWndLrc::OnElemEvent(Dui::CElem* pElem, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const auto* const pnm = (Dui::DUINMHDR*)lParam;
	if (pElem == &m_Lrc)
		switch (pnm->uCode)
		{
		case ELEN_DTLRC_GET_TIME:
		{
			const auto p = (NM_DTL_GET_TIME*)lParam;
			p->fTime = (float)App->GetPlayer().GetBass().GetPosition();
		}
		return 0;
		}
	if (pElem == &m_BTPrev)
		App->GetPlayer().Prev();
	else if (pElem == &m_BTPlay)
		App->GetPlayer().PlayOrPause();
	else if (pElem == &m_BTNext)
		App->GetPlayer().Next();
	else if (pElem == &m_BTLock)
		;
	else if (pElem == &m_BTClose)
		;
	return __super::OnElemEvent(pElem, uMsg, wParam, lParam);
}

LRESULT CWndLrc::OnRenderEvent(UINT uMsg, Dui::RENDER_EVENT& e)
{
	if (uMsg == Dui::RE_FILLBACKGROUND)
	{
		constexpr float MaxAlpha = 0.4f;
		if (m_bAnFade)
			BbrGet()->SetColor({ .a = m_AnFade.K * MaxAlpha });
		else if (m_bShowBk)
			BbrGet()->SetColor({ .a = MaxAlpha });
		else
			return Dui::RER_NONE;
		GetDeviceContext()->FillRectangle(e.FillBkg.rc, BbrGet());
		return Dui::RER_NONE;
	}
	return __super::OnRenderEvent(uMsg, e);
}

void CWndLrc::Tick(int iMs)
{
	if (m_bAnFade)
	{
		m_bAnFade = m_AnFade.Tick((float)iMs, 200);
		Redraw(FALSE);
	}
}