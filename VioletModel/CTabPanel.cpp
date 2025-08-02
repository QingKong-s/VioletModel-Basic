#include "pch.h"
#include "CWndMain.h"

LRESULT CTabPanel::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
	{
		if (wParam == (WPARAM)&m_TAB)
		{
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::TBLE_GETDISPINFO:
			{
				const auto p = (Dui::NMTBLDISPINFO*)lParam;
				const auto pWnd = (CWndMain*)GetWnd();
				switch (p->idx)
				{
				case 0:
					p->pszText = L"主页";
					p->cchText = 2;
					p->pImage = pWnd->RealizeImage(
						ShouldAppsUseDarkMode() ? GImg::HomeW : GImg::Home);
					break;
				case 1:
					p->pszText = L"列表";
					p->cchText = 2;
					p->pImage = pWnd->RealizeImage(
						ShouldAppsUseDarkMode() ? GImg::ListW : GImg::List);
					break;
				case 2:
					p->pszText = L"效果";
					p->cchText = 2;
					p->pImage = pWnd->RealizeImage(
						ShouldAppsUseDarkMode() ? GImg::PluginW : GImg::Plugin);
					break;
				case 3:
					p->pszText = L"设置";
					p->cchText = 2;
					p->pImage = pWnd->RealizeImage(
						ShouldAppsUseDarkMode() ? GImg::SettingsW : GImg::Settings);
					break;
				default:
					p->pszText = L"...";
					p->cchText = 3;
					p->pImage = pWnd->RealizeImage(GImg::SmallLogo);
				}
			}
			return 0;

			case Dui::EE_CLICK:
			{
				auto nm{ *(Dui::NMTBLITEMINDEX*)lParam };
				nm.uCode = ELEN_PAGE_CHANGE;
				GenElemNotify(&nm);
			}
			return 0;
			}
		}
	}
	break;

	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		m_pBrush->SetColor(App->GetColor(GPal::TabPanelBk));
		m_pDC->FillRectangle(ps.rcfClipInElem, m_pBrush);
		EndPaint(ps);
	}
	return 0;

	case WM_SIZE:
	{
		RECT rc;
		rc.left = rc.top = (GetWidth() - CxyWndLogo) / 2;
		rc.right = rc.left + CxyWndLogo;
		rc.bottom = rc.top + CxyWndLogo;
		m_LAIcon.SetRect(rc);

		const int Padding = (int)GetTheme()->GetMetrics(Dui::Metrics::SmallPadding);
		m_TAB.SetRect({ Padding, rc.bottom + 30, GetWidth() - Padding, GetHeight() - Padding });
	}
	break;

	case WM_CREATE:
		m_pDC->CreateSolidColorBrush({}, &m_pBrush);
		m_LAIcon.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, GetWidth(), GetWidth(), this, GetWnd());
		m_LAIcon.SetOnlyBitmap(TRUE);
		m_LAIcon.SetBitmap(((CWndMain*)GetWnd())->RealizeImage(GImg::WindowLogo));
		m_LAIcon.SetBkImgMode(eck::BkImgMode::StretchKeepAspectRatio);
		m_LAIcon.SetFullElem(TRUE);

		m_TAB.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, 0, 0, this, GetWnd());

		m_TAB.SetItemCount(4);
		break;

	case WM_DESTROY:
		SafeRelease(m_pBrush);
		break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}
