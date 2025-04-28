#include "pch.h"
#include "CWndMain.h"

void CPageList::OnPlayEvent(const PLAY_EVT_PARAM& e)
{

}

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
					p->cchText = swprintf((PWSTR)p->pszText, L"播放列表 %d", p->idx);
				if (p->uMask & eck::DIM_IMAGE)
					p->pImage = ((CWndMain*)GetWnd())->RealizeImg(GImg::List);
			}
			return 0;
			}
		else if (wParam == (WPARAM)&m_GLList)
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::LEE_GETDISPINFO:
			{
				const auto p = (Dui::LEE_DISPINFO*)lParam;
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
		m_EDSearch.SetTextFormat(GetTextFormat());
	}
	return 0;

	case WM_CREATE:
	{
		{
			m_EDSearch.Create(nullptr, Dui::DES_VISIBLE, 0,
				0, 0, 0, CyStdEdit, this);
			m_LytPlayList.Add(&m_EDSearch, { .cyBottomHeight = CxPageIntPadding }, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

			m_TBLPlayList.Create(nullptr, Dui::DES_VISIBLE, 0,
				0, 0, CxListFileList, 0, this, GetWnd());
			m_TBLPlayList.SetItemCount(40);
			m_TBLPlayList.SetBottomExtraSpace(CyPlayPanel);
			m_TBLPlayList.ReCalc();
			m_vList.resize(40);
			m_LytPlayList.Add(&m_TBLPlayList, {}, eck::LF_FILL, 1);
		}
		m_Lyt.Add(&m_LytPlayList, { .cxRightWidth = CxPageIntPadding },
			eck::LF_FIX_WIDTH | eck::LF_FILL_HEIGHT);

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
