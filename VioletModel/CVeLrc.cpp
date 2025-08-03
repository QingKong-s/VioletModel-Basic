#include "pch.h"
#include "CVeLrc.h"

enum
{
	IDT_MOUSEIDLE = 1001,
	TE_MOUSEIDLE = 500,
	T_MOUSEIDLEMAX = 4500,
};

constexpr inline float AnDurLrcSelBkg{ 100.f };			// 歌词选中背景动画时长
constexpr inline float AnDurLrcScrollExpand{ 200.f };	// 滚动展开动画时长
constexpr inline float AnDurLrcDelay{ 400.f };			// 每个项目的延迟动画时长
constexpr inline float DurMaxItemDelay{ 30.f };			// 延迟间隔

static constexpr D2D1_COLOR_F InterpolateColor(
	const D2D1_COLOR_F& c1, const D2D1_COLOR_F& c2, float k)
{
	return {
		c1.r + (c2.r - c1.r) * k,
		c1.g + (c2.g - c1.g) * k,
		c1.b + (c2.b - c1.b) * k,
		c1.a + (c2.a - c1.a) * k };
}

void CVeLrc::ScrAnProc(int iPos, int iPrevPos)
{
	if (IsEmpty())
		return;
	const auto iMs = m_psv->GetCurrTickInterval();
	if (m_AnEnlarge.IsEnd())
	{
		m_bEnlarging = FALSE;
		m_idxPrevAnItem = -1;
		m_fAnValue = m_fPlayingItemScale;
	}
	else
		m_fAnValue = m_AnEnlarge.Tick((float)iMs);

	EckAssert(AnDurLrcScrollExpand < (float)m_psv->GetDuration());
	if (m_bScrollExpand)
	{
		if (m_bSeEnlarging)
			m_msScrollExpand += iMs;
		else
			m_msScrollExpand -= iMs;
		m_kScrollExpand = eck::Easing::Linear(m_msScrollExpand, 1.f,
			m_fPlayingItemScale - 1.f, AnDurLrcScrollExpand);
		if (m_msScrollExpand >= AnDurLrcScrollExpand ||
			m_msScrollExpand <= 0.f)
		{
			m_bScrollExpand = FALSE;
			m_kScrollExpand = m_bSeEnlarging ? m_fPlayingItemScale : 1.f;
			m_msScrollExpand = m_bSeEnlarging ? AnDurLrcScrollExpand : 0.f;
		}
	}

	ScrDoItemScroll(iPos);

	ItmReCalcTop();
	InvalidateRect();
}

void CVeLrc::ItmReCalcTop()
{
	if (IsEmpty())
	{
		m_idxTop = 0;
		return;
	}
	m_idxTop = ItmIndexFromY(0.f);
	EckAssert(m_idxTop >= 0);
}

float CVeLrc::ItmPaint(int idx)
{
	EckAssert(idx >= 0 && idx < (int)m_vItem.size());
	const auto& Item = m_vItem[idx];
	const auto y = Item.y;
	D2D1_MATRIX_3X2_F Mat0;
	m_pDC->GetTransform(&Mat0);

	D2D1_POINT_2F ptScale{};
	ptScale.y = Item.cy / 2.f - m_cxyLineMargin;
	switch (m_eAlignH)
	{
	case eck::Align::Center:
		ptScale.x = GetWidthF() / 2.f;
		break;
	case eck::Align::Far:
		ptScale.x = GetWidthF();
		break;
	}

	D2D1_MATRIX_3X2_F Mat{ Mat0 };
	const auto cyExtra = (Item.cy - m_cxyLineMargin * 2.f) *
		(m_fPlayingItemScale - 1.f) / 2.f;
	Mat.dx += m_cxyLineMargin;
	Mat.dy += (y + m_cxyLineMargin + cyExtra);

	if (!Item.bCacheValid)
	{
		auto& Item = m_vItem[idx];
		constexpr float cyPadding[]{ 5.f,0.f };

		float x[2]{};
		switch (m_eAlignH)
		{
		case eck::Align::Center:
			x[0] = (GetWidthF() - GetWidthF() / m_fPlayingItemScale) / 2.f;
			x[1] = (GetWidthF() - Item.cxTrans) / 2.f;
			break;
		case eck::Align::Far:
			x[0] = (GetWidthF() - GetWidthF() / m_fPlayingItemScale);
			x[1] = (GetWidthF() - Item.cxTrans);
			break;
		}

		ComPtr<ID2D1PathGeometry> pPathGeometry;
		eck::GetTextLayoutPathGeometry(&Item.pLayout, 2, cyPadding,
			m_pDC, x, 0.f, pPathGeometry.RefOf());
		float xDpi, yDpi;
		m_pDC->GetDpi(&xDpi, &yDpi);
		m_pDC1->CreateFilledGeometryRealization(
			pPathGeometry.Get(),
			D2D1::ComputeFlatteningTolerance(
				D2D1::Matrix3x2F::Identity(), xDpi, yDpi, m_fPlayingItemScale),
			Item.pGr.AddrOfClear());
		Item.bCacheValid = TRUE;
	}

	Dui::State eState;
	if (m_vItem[idx].bSel)
		if (idx == m_idxHot)
			eState = Dui::State::HotSelected;
		else
			eState = Dui::State::Selected;
	else
		if (idx == m_idxHot || Item.bAnSelBkg)
			eState = Dui::State::Hot;
		else
			eState = Dui::State::None;
	if (eState != Dui::State::None)
	{
		D2D1_RECT_F rc;
		ItmGetRect(idx, rc);
		Dui::DTB_OPT Opt;
		if (Item.bAnSelBkg && eState == Dui::State::Hot)
		{
			const auto dxy = -m_cxyLineMargin * Item.kAnSelBkg;
			eck::InflateRect(rc, dxy, dxy);
			Opt.uFlags = Dui::DTBO_NEW_OPACITY;
			Opt.fOpacity = 1.f - Item.kAnSelBkg;
		}
		else
			Opt.uFlags = Dui::DTBO_NONE;
		GetTheme()->DrawBackground(Dui::Part::ListItem, eState, rc, &Opt);
	}
	if (idx == m_idxPrevAnItem)
	{
		const auto k = m_fPlayingItemScale + 1.f - m_fAnValue;
		m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
		m_pBrush->SetColor(InterpolateColor(
			m_Color[CriNormal], m_Color[CriHiLight], 1.f - m_fAnValue));
	}
	else if (idx == m_idxCurrAnItem)
	{
		const auto k = m_fAnValue;
		m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
		m_pBrush->SetColor(InterpolateColor(
			m_Color[CriNormal], m_Color[CriHiLight], m_fAnValue));
	}
	else
	{
		if (m_bScrollExpand || MiIsIdle())
		{
			m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(
				m_kScrollExpand, m_kScrollExpand, ptScale) * Mat);
		}
		else
			m_pDC->SetTransform(Mat);
		m_pBrush->SetColor(m_Color[m_idxCurr == idx ? CriHiLight : CriNormal]);
	}
	m_pDC1->DrawGeometryRealization(Item.pGr.Get(), m_pBrush);
	m_pDC->SetTransform(Mat0);
#ifdef _DEBUG
	WCHAR szDbg[eck::CchI32ToStrBufNoRadix2];
	const auto cchDbg = swprintf_s(szDbg, L"%d", idx);
	D2D1_RECT_F rcDbg;
	ItmGetRect(idx, rcDbg);
	m_pBrush->SetColor(D2D1::ColorF{ (ItmIsDelaying() && ItmInDelayRange(idx)) ?
		D2D1::ColorF::Green : D2D1::ColorF::Red });
	m_pDC->DrawTextW(szDbg, cchDbg, GetTextFormat(), rcDbg, m_pBrush);
#endif
	return y;
}

void CVeLrc::MiBeginDetect()
{
	if (!m_tMouseIdle)
		SetTimer(IDT_MOUSEIDLE, TE_MOUSEIDLE);
	m_tMouseIdle = T_MOUSEIDLEMAX;
}

int CVeLrc::ItmHitTest(POINT pt)
{
	RECT rc;
	for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
	{
		ItmGetRect(i, rc);
		if (eck::PtInRect(rc, pt))
			return i;
		if (rc.top > GetHeight())
			break;
	}
	return -1;
}

void CVeLrc::ItmGetRect(int idx, _Out_ RECT& rc)
{
	const auto& e = m_vItem[idx];
	rc.left = (long)e.x;
	rc.top = (long)e.y;
	rc.right = long(e.x + e.cx);
	rc.bottom = long(rc.top + e.cy);
}

void CVeLrc::ItmGetRect(int idx, _Out_ D2D1_RECT_F& rc)
{
	const auto& e = m_vItem[idx];
	rc.left = e.x;
	rc.top = e.y;
	rc.right = e.x + e.cx;
	rc.bottom = rc.top + e.cy;
}

int CVeLrc::ItmIndexFromY(float y)
{
	if (IsEmpty())
		return -1;
	if (m_bItemAnDelay)
	{
		for (int i = (int)m_vItem.size() - 1; i >= 0; --i)
		{
			if (y > m_vItem[i].y)
				return i;
		}
		return 0;
	}
	else
	{
		const auto it = std::lower_bound(m_vItem.begin(), m_vItem.end(), y,
			[](const ITEM& r, float f) { return r.y < f; });
		if (it == m_vItem.end())
			return -1;
		if (it == m_vItem.begin())
			return 0;
		else
			return (int)std::distance(m_vItem.begin(), it - 1);
	}
}

void CVeLrc::ItmInvalidate(int idx)
{
	D2D1_RECT_F rc;
	ItmGetRect(idx, rc);
	ElemToClient(rc);
	InvalidateRect(rc);
}

void CVeLrc::SeBeginExpand(BOOL bEnlarge)
{
	const auto bWake = !IsValid();
	if (!m_bScrollExpand)
	{
		m_bScrollExpand = TRUE;
		if (m_bSeEnlarging = bEnlarge)
			m_msScrollExpand = 0.f;
		else
			m_msScrollExpand = AnDurLrcScrollExpand;
	}
	if (bWake)
		GetWnd()->WakeRenderThread();
}

void CVeLrc::ItmDelayPrepare(float dy)
{
	float y;
	m_bItemAnDelay = TRUE;// 马上要启动滚动条时间线，无需唤醒渲染线程
	m_bDelayScrollUp = (dy > 0.f);
	int i;
	const auto& ItemCurr = m_vItem[m_idxCurr];
	// 当前（含）以上
	y = ItmGetCurrentItemTarget() - ItemCurr.cy / 2.f
		+ ItemCurr.cy + m_cyLinePadding;
	m_idxDelayBegin = 0;
	for (int i = m_idxCurr; i >= 0; --i)
	{
		auto& e = m_vItem[i];
		y -= (e.cy + m_cyLinePadding);
		e.yAnDelayDst = y;
		e.yAnDelaySrc = e.y;
		e.msDelay = e.msAnDelay = 0.f;
		if (y - m_cyLinePadding <= 0.f)
		{
			m_idxDelayBegin = i;
			break;
		}
	}
	// 当前以下
	y = ItemCurr.yAnDelayDst + ItemCurr.cy + m_cyLinePadding;
	m_idxDelayEnd = (int)m_vItem.size() - 1;
	for (i = m_idxCurr + 1; i < (int)m_vItem.size(); ++i)
	{
		auto& e = m_vItem[i];
		e.yAnDelayDst = y;
		e.yAnDelaySrc = e.y;
		e.msDelay = e.msAnDelay = 0.f;
		y += (e.cy + m_cyLinePadding);
		if (y >= GetHeightF())
		{
			m_idxDelayEnd = i;
			break;
		}
	}
}

void CVeLrc::ItmDelayComplete()
{
	m_idxDelayBegin = m_idxDelayEnd = -1;
	m_bItemAnDelay = FALSE;
}

LRESULT CVeLrc::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);

		if (m_vItem.empty())
		{
			if (m_pGrEmptyText)
				m_pDC1->DrawGeometryRealization(m_pGrEmptyText, m_pBrush);
		}
		else
			for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
			{
				if (ItmPaint(i) > GetHeightF())
					break;
			}
		ECK_DUI_DBG_DRAW_FRAME;
		EndPaint(ps);
	}
	return 0;

	case WM_MOUSEMOVE:
	{
		ECK_DUILOCK;
		if (m_vItem.empty())
			break;
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ClientToElem(pt);

		int idx = ItmHitTest(pt);
		if (idx != m_idxHot)
		{
			std::swap(idx, m_idxHot);
			if (idx >= 0)
			{
				m_vItem[idx].OnKillHot();
				IsbWakeRenderThread();
			}
			if (m_idxHot >= 0)
			{
				m_vItem[m_idxHot].OnSetHot();
				IsbWakeRenderThread();
			}
		}
	}
	return 0;

	case WM_MOUSELEAVE:
	{
		ECK_DUILOCK;
		if (m_vItem.empty())
			break;
		int idx = -1;
		if (idx != m_idxHot)
		{
			std::swap(idx, m_idxHot);
			if (idx >= 0)
			{
				m_vItem[idx].OnKillHot();
				IsbWakeRenderThread();
			}
		}
	}
	return 0;

	case WM_MOUSEWHEEL:
	{
		ECK_DUILOCK;
		if (m_vItem.empty())
			break;
		if (!MiIsIdle())
			SeBeginExpand(TRUE);
		ScrManualScrolling();
		m_psv->OnMouseWheel2(-GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
		GetWnd()->WakeRenderThread();
	}
	return 0;

	case WM_LBUTTONDOWN:
	{
		ECK_DUILOCK;
		SetFocus();
		if (m_vItem.empty())
			break;
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ClientToElem(pt);

		int idx = ItmHitTest(pt);
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			if (idx >= 0)
				m_idxMark = idx;
		}
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			if (m_idxMark < 0 || idx < 0)
				break;
			const int idx0 = std::min(m_idxMark, idx);
			const int idx1 = std::max(m_idxMark, idx);
			int i = 0;
			for (; i < idx0; ++i)
			{
				if (m_vItem[i].bSel)
				{
					m_vItem[i].bSel = FALSE;
					ItmInvalidate(i);
				}
			}
			for (; i <= idx1; ++i)
			{
				if (!m_vItem[i].bSel)
				{
					m_vItem[i].bSel = TRUE;
					ItmInvalidate(i);
				}
			}
			for (; i < (int)m_vItem.size(); ++i)
			{
				if (m_vItem[i].bSel)
				{
					m_vItem[i].bSel = FALSE;
					ItmInvalidate(i);
				}
			}
			break;
		}
		else
		{
			if (idx >= 0)
				m_idxMark = idx;
			EckCounter((int)m_vItem.size(), i)
			{
				if (m_vItem[i].bSel)
				{
					m_vItem[i].bSel = FALSE;
					ItmInvalidate(i);
				}
			}
		}

		if (idx >= 0)
			if (!m_vItem[idx].bSel)
			{
				m_vItem[idx].bSel = TRUE;
				ItmInvalidate(idx);
			}
	}
	return 0;

	case WM_KEYDOWN:
	{
		if (m_vItem.empty())
			break;
		if (wParam == VK_ESCAPE)
		{
			m_tMouseIdle = 0;
			KillTimer(IDT_MOUSEIDLE);
			ScrAutoScrolling();
		}
	}
	return 0;

	case WM_NOTIFY:
	{
		if ((Dui::CElem*)wParam == &m_SB)
		{
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::EE_VSCROLL:
			{
				ECK_DUILOCK;
				if (!MiIsIdle())
					SeBeginExpand(TRUE);
				ScrManualScrolling();
				m_psv->InterruptAnimation();
				ScrDoItemScroll(m_psv->GetPos());
				ItmReCalcTop();
				InvalidateRect();
			}
			return TRUE;
			}
		}
	}
	return 0;

	case WM_TIMER:
	{
		if (wParam == IDT_MOUSEIDLE)
		{
			ECK_DUILOCK;
			if (m_vItem.empty())
				break;
			m_tMouseIdle -= TE_MOUSEIDLE;
			if (m_tMouseIdle <= 0)
			{
				m_tMouseIdle = 0;
				KillTimer(IDT_MOUSEIDLE);
				ScrAutoScrolling();
			}
		}
	}
	return 0;

	case WM_SIZE:
	{
		ItmDelayComplete();
		ItmLayout();
		RECT rc;
		rc.left = GetWidth() - m_SB.GetWidth();
		rc.top = 0;
		rc.right = rc.left + m_SB.GetWidth();
		rc.bottom = rc.top + GetHeight();
		m_SB.SetRect(rc);
		if (!IsEmpty())
		{
			if (MiIsIdle())
				ScrFixItemPosition();
			else
			{
				const auto idxCurr = m_idxCurr < 0 ? 0 : m_idxCurr;
				const auto& CurrItem = m_vItem[idxCurr];
				float y = ItmGetCurrentItemTarget() - CurrItem.cy / 2.f
					+ CurrItem.cy + m_cyLinePadding;
				for (int i = idxCurr; i >= 0; --i)
				{
					auto& e = m_vItem[i];
					y -= (e.cy + m_cyLinePadding);
					e.y = e.yNoDelay = y;
				}
				m_psv->SetPos(-(int)m_vItem.front().y);
				y = CurrItem.yNoDelay + CurrItem.cy + m_cyLinePadding;
				for (int i = idxCurr + 1; i < (int)m_vItem.size(); ++i)
				{
					auto& e = m_vItem[i];
					e.y = e.yNoDelay = y;
					y += (e.cy + m_cyLinePadding);
				}
			}
			ItmReCalcTop();
		}
	}
	break;

	case WM_CREATE:
	{
		GetWnd()->RegisterTimeLine(this);

		m_pDC->QueryInterface(&m_pDC1);
		m_pDC->CreateSolidColorBrush({}, &m_pBrush);

		m_SB.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, (int)GetTheme()->GetMetrics(Dui::Metrics::CxVScroll), 0,
			this, GetWnd());
		m_psv = m_SB.GetScrollView();
		m_psv->AddRef();
		m_psv->SetMinThumbSize(Dui::CxyMinScrollThumb);
		m_psv->SetCallBack([](int iPos, int iPrevPos, LPARAM lParam)
			{
				((CVeLrc*)lParam)->ScrAnProc(iPos, iPrevPos);
			}, (LPARAM)this);
		m_psv->SetDelta(80);
	}
	break;

	case WM_DESTROY:
	{
		SafeRelease(m_pBrush);
		SafeRelease(m_pTextFormat);
		SafeRelease(m_psv);
		SafeRelease(m_pDC1);
		SafeRelease(m_pGrEmptyText);
		m_vItem.clear();
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}

HRESULT CVeLrc::LrcUpdateEmptyText(std::wstring_view svEmptyText)
{
	ECK_DUILOCK;
	HRESULT hr;
	SafeRelease(m_pGrEmptyText);

	ComPtr<IDWriteTextLayout> pLayout;
	hr = eck::g_pDwFactory->CreateTextLayout(svEmptyText.data(),
		(UINT32)svEmptyText.size(), m_pTextFormat,
		GetWidthF(), GetHeightF(), &pLayout);
	if (FAILED(hr)) return hr;
	pLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

	ComPtr<ID2D1PathGeometry1> pPathGeometry;
	eck::GetTextLayoutPathGeometry(pLayout.Get(), m_pDC,
		0.f, 0.f, pPathGeometry.RefOf());
	float xDpi, yDpi;
	m_pDC->GetDpi(&xDpi, &yDpi);

	return m_pDC1->CreateFilledGeometryRealization(
		pPathGeometry.Get(),
		D2D1::ComputeFlatteningTolerance(
			D2D1::Matrix3x2F::Identity(), xDpi, yDpi),
		&m_pGrEmptyText);
}

HRESULT CVeLrc::LrcSetCurrentLine(int idxCurr)
{
	if (idxCurr < 0)
		return E_BOUNDS;
	if (m_idxCurr == idxCurr)
		return S_FALSE;
#ifdef _DEBUG
	if (idxCurr >= (int)m_vItem.size())
	{
		EckDbgPrintFmt(ECK_FUNCTIONW L"\n\t索引越界：%d", idxCurr);
		EckDbgBreak();
	}
#endif
	ECK_DUILOCK;
	const int idxPrev = m_idxCurr;
	m_idxCurr = idxCurr;
	if (MiIsIdle())
	{
		if (idxPrev >= 0)
			ItmInvalidate(idxPrev);
		if (m_idxCurr >= 0)
			ItmInvalidate(m_idxCurr);
	}
	else
	{
		m_bEnlarging = TRUE;
		m_idxPrevAnItem = idxPrev;
		m_idxCurrAnItem = m_idxCurr;
		m_AnEnlarge.Begin(1.f, m_fPlayingItemScale - 1.f, (float)m_psv->GetDuration());
		const auto& e = m_vItem[m_idxCurr];
		const auto dy = (e.yNoDelay + e.cy / 2.f) - ItmGetCurrentItemTarget();
		m_psv->InterruptAnimation();
		m_psv->SmoothScrollDelta((int)dy);
		ItmDelayPrepare(dy);
		GetWnd()->WakeRenderThread();
	}
	return S_OK;
}

HRESULT CVeLrc::LrcInit(std::shared_ptr<std::vector<eck::LRCINFO>> pvLrc)
{
	ECK_DUILOCK;
	m_pvLrc = pvLrc;
	m_idxCurr = -1;
	ItmLayout();
	ItmDelayComplete();
	if (!IsEmpty())
		ScrFixItemPosition();
	ItmReCalcTop();
	InvalidateRect();
	//ScrAutoScrolling();
	return S_OK;
}

void CVeLrc::LrcClear()
{
	ECK_DUILOCK;
	m_pvLrc.reset();
	m_vItem.clear();
	m_idxTop = -1;
	m_idxHot = -1;
	m_idxMark = -1;
	m_idxCurr = -1;
	m_idxPrevAnItem = -1;
	m_idxCurrAnItem = -1;
	m_fAnValue = 1.f;
	m_bEnlarging = FALSE;
	InvalidateRect();
}

void CVeLrc::SetTextFormatTrans(IDWriteTextFormat* pTf)
{
	ECK_DUILOCK;
	std::swap(m_pTextFormatTrans, pTf);
	if (m_pTextFormatTrans)
		m_pTextFormatTrans->AddRef();
	if (pTf)
		pTf->Release();
	CallEvent(WM_SETFONT, 0, 0);
}

void CVeLrc::Tick(int iMs)
{
	BOOL bAn{}, bDelay{};
	int i = (m_bDelayScrollUp ? 0 : (int)m_vItem.size() - 1);
	for (; i >= 0 && i < (int)m_vItem.size(); (m_bDelayScrollUp ? ++i : --i))
	{
		auto& e = m_vItem[i];
		if (e.bAnSelBkg)
		{
			if (e.bAnSelBkgEnlarge)
				e.msAnSelBkg -= iMs;
			else
				e.msAnSelBkg += iMs;
			e.kAnSelBkg = eck::Easing::Linear(
				e.msAnSelBkg, 0.f, 1.f, AnDurLrcSelBkg);
			if (e.kAnSelBkg >= 1.f || e.kAnSelBkg <= 0.f)
				e.bAnSelBkg = FALSE;
			else
				bAn = TRUE;
			ItmInvalidate(i);
		}
		if (m_bItemAnDelay && !bDelay && ItmInDelayRange(i))
			if (e.msDelay >= DurMaxItemDelay)
			{
				e.msAnDelay += iMs;
				auto k = eck::Easing::OutCubic(
					e.msAnDelay, 0.f, 1.f, AnDurLrcDelay);
				if (e.msAnDelay >= AnDurLrcDelay)// 动画结束
				{
					k = 1.f;
					e.msDelay = 0.f;
					e.msAnDelay = 0.f;
					if (m_bDelayScrollUp)
					{
						EckAssert(m_idxDelayBegin == i);
						++m_idxDelayBegin;
					}
					else
					{
						EckAssert(m_idxDelayEnd == i);
						--m_idxDelayEnd;
					}
					if (m_idxDelayEnd < m_idxDelayBegin)
						ItmDelayComplete();
				}
				D2D1_RECT_F rc;
				ItmGetRect(i, rc);
				e.y = e.yAnDelaySrc + (e.yAnDelayDst - e.yAnDelaySrc) * k;
				rc.top = std::min(rc.top, e.y);
				rc.bottom = std::max(rc.bottom, e.y + e.cy);

				ElemToClient(rc);
				InvalidateRect(rc);
			}
			else
			{
				e.msDelay += iMs;
				bDelay = TRUE;
			}
	}
	m_bAnSelBkg = bAn;
	if (m_bItemAnDelay)
		ItmReCalcTop();
}

void CVeLrc::ScrAutoScrolling()
{
	if (m_idxCurr != m_idxCurrAnItem && m_idxCurrAnItem >= 0)
	{
		m_idxPrevAnItem = m_idxCurrAnItem;
		m_idxCurrAnItem = m_idxCurr;
		m_bEnlarging = TRUE;
		m_AnEnlarge.Begin(1.f, m_fPlayingItemScale - 1.f, (float)m_psv->GetDuration());
	}
	const auto& CurrItem = m_idxCurr < 0 ? m_vItem.front() : m_vItem[m_idxCurr];
	const auto dy = (CurrItem.yNoDelay + CurrItem.cy / 2.f) - ItmGetCurrentItemTarget();
	m_psv->InterruptAnimation();
	m_psv->SmoothScrollDelta((int)dy);
	SeBeginExpand(FALSE);
	if (m_idxCurr >= 0)
		ItmDelayPrepare(dy);
	GetWnd()->WakeRenderThread();
}

void CVeLrc::ItmLayout()
{
	const float cx = GetWidthF(), cy = GetHeightF();
	if (cx <= 0.f || cy <= 0.f)
		return;
	m_vItem.clear();
	if (!m_pvLrc || m_pvLrc->empty())
		return;
	const auto& vLrc = *m_pvLrc;

	constexpr auto cyMainTransPadding = 5.f;

	m_vItem.resize(vLrc.size());
	const float cxMax = (cx - m_cxyLineMargin * 2.f) / m_fPlayingItemScale;
	DWRITE_TEXT_METRICS Metrics;
	const auto yInit = (float)-m_psv->GetPos();
	float y = yInit;
	EckCounter(vLrc.size(), i)
	{
		auto& e = m_vItem[i];
		eck::g_pDwFactory->CreateTextLayout(vLrc[i].pszLrc, vLrc[i].cchLrc,
			m_pTextFormat, cxMax, cy, &e.pLayout);
		e.pLayout->GetMetrics(&Metrics);
		e.y = e.yNoDelay = y;
		e.cx = Metrics.width;
		e.cy = Metrics.height;

		if (vLrc[i].pszTranslation)
		{
			eck::g_pDwFactory->CreateTextLayout(vLrc[i].pszTranslation,
				vLrc[i].cchTotal - vLrc[i].cchLrc,
				m_pTextFormatTrans, cxMax, cy, &e.pLayoutTrans);
			e.pLayoutTrans->GetMetrics(&Metrics);
			e.cxTrans = Metrics.width;
			e.cx = std::max(e.cx, Metrics.width);
			e.cy += (Metrics.height + cyMainTransPadding);
		}

		switch (m_eAlignH)
		{
		case eck::Align::Center:
			e.x = (cx - e.cx) / 2.f;
			break;
		case eck::Align::Far:
			e.x = cx - e.cx;
			break;
		}
		e.cx *= m_fPlayingItemScale;
		e.cy *= m_fPlayingItemScale;
		e.cx += (m_cxyLineMargin * 2.f);
		e.cy += (m_cxyLineMargin * 2.f);
		y += (e.cy + m_cyLinePadding);
	}

	m_psv->SetMin(int(-cy / 3.f - m_vItem.front().cy));
	m_psv->SetMax(int(y - yInit - m_cyLinePadding + cy / 3.f * 2.f));
	m_psv->SetPage((int)cy);
	m_psv->SetViewSize((int)cy);
}

void CVeLrc::ScrManualScrolling()
{
	MiBeginDetect();
	m_bEnlarging = FALSE;
	m_idxPrevAnItem = -1;
	m_fAnValue = m_fPlayingItemScale;
}

void CVeLrc::ScrDoItemScroll(int iPos)
{
	float y = (float)-iPos;
	auto& Front = m_vItem.front();
	Front.yNoDelay = y;
	if (!(m_bItemAnDelay && ItmInDelayRange(0)))
		Front.y = y;
	for (int i = 1; i < (int)m_vItem.size(); ++i)
	{
		const auto& Prev = m_vItem[i - 1];
		y += (Prev.cy + m_cyLinePadding);
		if (!(m_bItemAnDelay && ItmInDelayRange(i)))
			m_vItem[i].y = y;
		m_vItem[i].yNoDelay = y;
	}
}

void CVeLrc::ScrFixItemPosition()
{
	const auto& e = m_vItem.front();
	const auto iRealPos = (int)-e.y;
	if (iRealPos < m_psv->GetMin())
		ScrDoItemScroll(m_psv->GetPos());
	else if (iRealPos > m_psv->GetMaxWithPage())
		ScrDoItemScroll(m_psv->GetPos());
}


void CVeLrc::ITEM::OnSetHot()
{
	if (!bAnSelBkg)
	{
		bAnSelBkg = TRUE;
		msAnSelBkg = AnDurLrcSelBkg;
	}
	bAnSelBkgEnlarge = TRUE;
}

void CVeLrc::ITEM::OnKillHot()
{
	if (!bAnSelBkg)
	{
		bAnSelBkg = TRUE;
		msAnSelBkg = 0.f;
	}
	bAnSelBkgEnlarge = FALSE;
}