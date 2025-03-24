#include "pch.h"

#include "CWndMain.h"

void CWndMain::ClearRes()
{
	for (auto& e : m_vBmpRealization)
		SafeRelease(e);
}

BOOL CWndMain::OnCreate(HWND hWnd, CREATESTRUCT* pcs)
{
	eck::GetThreadCtx()->UpdateDefColor();
	eck::EnableWindowMica(hWnd);
	const MARGINS mg{ -1 };
	DwmExtendFrameIntoClientArea(hWnd, &mg);

	m_pTfLeft = eck::CreateDefTextFormatWithSize(15);
	m_pTfLeft->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	m_pTfCenter = eck::CreateDefTextFormatWithSize(15);
	m_pTfCenter->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTfCenter->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	LOGFONTW lf;
	eck::GetDefFontInfo(lf, 96);
	IDWriteTextFormat* pTextFormat;
	const auto hr = eck::g_pDwFactory->CreateTextFormat(
		lf.lfFaceName,
		nullptr,
		(DWRITE_FONT_WEIGHT)600,
		lf.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		20,
		L"zh-cn",
		&pTextFormat);

	m_TabPanel.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);

	m_LAPageTitle.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_LAPageTitle.SetTextFormat(pTextFormat);
	pTextFormat->Release();

	m_PageMain.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_PageMain.SetTextFormat(m_pTfCenter);

	m_PageList.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_PageList.SetTextFormat(m_pTfLeft);

	m_PlayPanel.Create(nullptr, Dui::DES_VISIBLE/* | Dui::DES_BLURBKG*/, 0,
		0, 0, 0, 0, nullptr, this);

	m_TBProgress.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_TBProgress.SetRange(0, 100);
	m_TBProgress.SetPos(50);
	m_TBProgress.SetTrackSize(CyProgressTrack);

	m_TitleBar.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);

	ShowPage(Page::List, FALSE);
	return TRUE;
}

void CWndMain::ShowPage(Page ePage, BOOL bAnimate)
{
	__assume(ePage < Page::Max);
	const int idxShow = (int)ePage;

	ClearPageAnimation();

	const auto bAlreadyVisible =
		(m_vPage[idxShow]->GetStyle() & Dui::DES_VISIBLE);

	m_LAPageTitle.SetText(MainWndPageName[idxShow]);
	m_LAPageTitle.InvalidateRect();
	m_vPage[idxShow]->SetVisible(TRUE);
	for (int i = 0; i < idxShow; ++i)
		m_vPage[i]->SetVisible(FALSE);
	for (int i = idxShow + 1; i < (int)Page::Max; ++i)
		m_vPage[i]->SetVisible(FALSE);

	if (bAnimate)
	{
		if (bAlreadyVisible)
			return;
		m_pAnPage = m_vPage[idxShow];
		if (!m_pecPage)
		{
			m_pecPage = new eck::CEasingCurve{};
			RegisterTimeLine(m_pecPage);
			m_pecPage->SetParam((LPARAM)this);
			m_pecPage->SetDuration(160);
			m_pecPage->SetAnProc(eck::Easing::OutCubic);
			m_pecPage->SetCallBack([](float fCurrValue, float fOldValue, LPARAM lParam)
				{
					auto p = (CWndMain*)lParam;
					const auto x = p->m_pAnPage->GetRectF().left;
					p->m_pAnPage->SetPos(x, CyPageTitle + 12 + 40 - fCurrValue);
				});
		}
		m_pecPage->SetRange(0, 40);
		m_pecPage->Begin();
		WakeRenderThread();
	}
}

void CWndMain::ClearPageAnimation()
{
	if (!m_pAnPage)
		return;
	m_pecPage->End();
	m_pAnPage->SetStyle(m_pAnPage->GetStyle() & ~Dui::DES_COMPOSITED);
	m_pAnPage = nullptr;
}

CWndMain::~CWndMain()
{
}

LRESULT CWndMain::OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
	{
		ClearPageAnimation();
		const auto cxClient = Phy2Log(LOWORD(lParam));
		const auto cyClient = Phy2Log(HIWORD(lParam));
		m_TitleBar.SetRect({ 0,0,cxClient,CyTitleBar });
		m_TabPanel.SetRect({ 0,0,CxTabPanel,cyClient - CyPalyPanel });

		const auto yPlayPanel = cyClient - CyPalyPanel;
		m_PlayPanel.SetRect({ 0,cyClient - CyPalyPanel,cxClient,cyClient });

		const auto dTrackSpacing = m_TBProgress.GetTrackSpacing();
		m_TBProgress.SetRect({ 
			-(int)dTrackSpacing,
			yPlayPanel - CyProgress / 2,
			cxClient + (int)dTrackSpacing,
			yPlayPanel + CyProgress / 2 });

		m_LAPageTitle.SetRect({
			CxTabPanel + CxTabToPagePadding,
			12,
			cxClient,
			CyPageTitle });
		for (auto& e : m_vPage)
			e->SetRect({
				CxTabPanel + CxTabToPagePadding,
				CyPageTitle + 12,
				cxClient,
				cyClient - CxTabToPagePadding });
	}
	break;

	case WM_NCCALCSIZE:
	{
		const auto cxFrame = eck::DaGetSystemMetrics(SM_CXFRAME, GetDpiValue());
		const auto cyFrame = eck::DaGetSystemMetrics(SM_CYFRAME, GetDpiValue());
		return eck::MsgOnNcCalcSize(wParam, lParam, { cxFrame,cxFrame,0,cyFrame });
	}
	break;

	case WM_CREATE:
		__super::OnMsg(hWnd, uMsg, wParam, lParam);
		return HANDLE_WM_CREATE(hWnd, wParam, lParam, OnCreate);
	case WM_DESTROY:
		__super::OnMsg(hWnd, uMsg, wParam, lParam);
		ClearRes();
		PostQuitMessage(0);
		return 0;
	case WM_SYSCOLORCHANGE:
		eck::MsgOnSysColorChangeMainWnd(hWnd, wParam, lParam);
		break;
	case WM_SETTINGCHANGE:
	{
		eck::MsgOnSettingChangeMainWnd(hWnd, wParam, lParam);
		if (eck::IsColorSchemeChangeMessage(lParam))
		{
			const auto bDark = ShouldAppsUseDarkMode();
			App->SetDarkMode(bDark);
			SwitchStdThemeMode(bDark);
			Redraw();
		}
	}
	break;
	}
	return __super::OnMsg(hWnd, uMsg, wParam, lParam);
}

BOOL CWndMain::PreTranslateMessage(const MSG& Msg)
{
	return __super::PreTranslateMessage(Msg);
}

LRESULT CWndMain::OnElemEvent(Dui::CElem* pElem, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case ELEN_PAGE_CHANGE:
	{
		const auto* const p = (Dui::LTN_ITEM*)lParam;
		ShowPage((Page)p->idxItem, TRUE);
	}
	return 0;
	}
	return __super::OnElemEvent(pElem, uMsg, wParam, lParam);
}
