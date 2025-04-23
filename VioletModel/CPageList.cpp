#include "pch.h"
#include "CWndMain.h"

LRESULT CPageList::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
	{
		if (wParam == (WPARAM)&m_TBLPlayList)
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::TBLE_GETDISPINFO:
			{
				const auto p = (Dui::TBL_DISPINFO*)lParam;
				if (p->uMask & eck::DIM_TEXT)
				{
					p->cchText = swprintf((PWSTR)p->pszText, L"播放列表 %d", p->idx);
					p->ppTextLayout = &m_vList[p->idx].pTextLayout;
				}
				if (p->uMask & eck::DIM_IMAGE)
					p->pImage = ((CWndMain*)GetWnd())->RealizeImg(GImg::List);
			}
			return 0;
			}
		else if (wParam == (WPARAM)&m_GLList)
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::GLE_GETDISPINFO:
			{
				const auto p = (Dui::GL_GETDISPINFO*)lParam;

			}
			return 0;
			}
	}
	return 0;

	case WM_SIZE:
		m_Lyt.Arrange(0, 0, GetWidth(), GetHeight());
		break;

	case WM_SETFONT:
	{
		m_TBLPlayList.SetTextFormat(GetTextFormat());
		m_GLList.SetTextFormat(GetTextFormat());
	}
	return 0;

	case WM_CREATE:
	{
		m_TBLPlayList.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxListFileList, 0, this, GetWnd());
		m_Lyt.Add(&m_TBLPlayList, { .cxRightWidth = CxPageIntPadding },
			eck::LF_FIX_WIDTH | eck::LF_FILL_HEIGHT);
		m_TBLPlayList.SetItemCount(40);
		m_TBLPlayList.SetBottomPadding(CyPlayPanel);
		m_vList.resize(40);

		m_GLList.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, 0, 0, this, GetWnd());
		m_Lyt.Add(&m_GLList, {}, eck::LF_FILL, 1);
	}
	break;
	case WM_DESTROY:
	{
		m_vList.clear();
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}
