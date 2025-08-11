#include "pch.h"
#include "CWndLrc.h"
#include "CWndMain.h"


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
			if (!GetCurrNcHitElem())
				lResult = HTCAPTION;
		}
		return lResult;
	}
	break;
	case WM_SIZE:
	{
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);
		const auto cxLyt = m_Layout.LoGetSize().cx;
		m_Layout.Arrange((GetClientWidthLog() - cxLyt) / 2, 0,
			cxLyt, CxyLrcBtn);
		return lResult;
	}
	break;
	case WM_CREATE:
	{
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);

		m_pVioletTheme = new CVioletTheme{};
		m_pVioletTheme->Init(GetDeviceContext(), GetStdTheme());

		ID2D1Bitmap1* pBitmap;

		m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTPrev.SetBitmap(App->GetMainWindow().RealizeImage(GImg::PrevSolid));
		m_BTPrev.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTPrev, { .cxRightWidth = CxLrcPadding }, eck::LF_FIX);

		m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTPlay.SetBitmap(App->GetMainWindow().RealizeImage(GImg::TriangleSolid));
		m_BTPlay.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTPlay, { .cxRightWidth = CxLrcPadding }, eck::LF_FIX);

		m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 2, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTNext.SetBitmap(App->GetMainWindow().RealizeImage(GImg::NextSolid));
		m_BTNext.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTNext, { .cxRightWidth = CxLrcPadding }, eck::LF_FIX);

		m_BTLock.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 3, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTLock.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTLock, {}, eck::LF_FIX);

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

LRESULT CWndLrc::OnRenderEvent(UINT uMsg, Dui::RENDER_EVENT& e)
{
	if (uMsg == Dui::RE_FILLBACKGROUND)
	{
		GetBkgBrush()->SetColor({ .a = 0.2f });
		GetDeviceContext()->FillRectangle(e.FillBkg.rc, GetBkgBrush());
		return Dui::RER_NONE;
	}
	return __super::OnRenderEvent(uMsg, e);
}