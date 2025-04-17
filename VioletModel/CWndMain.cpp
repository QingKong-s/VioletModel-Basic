#include "pch.h"

#include "CWndMain.h"

void CWndMain::ClearRes()
{
	for (auto& e : m_vBmpRealization)
		SafeRelease(e);
}

BOOL CWndMain::OnCreate(HWND hWnd, CREATESTRUCT* pcs)
{
	CBass::Init();
	App->GetPlayer().GetSignal().Connect(this, &CWndMain::OnPlayEvent);
	eck::GetThreadCtx()->UpdateDefColor();
	eck::EnableWindowMica(hWnd);
	const MARGINS mg{ -1 };
	DwmExtendFrameIntoClientArea(hWnd, &mg);

	BlurInit();
	BlurSetUseLayer(TRUE);

	m_pTfLeft = eck::CreateDefTextFormatWithSize(15);
	m_pTfLeft->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	m_pTfCenter = eck::CreateDefTextFormatWithSize(15);
	m_pTfCenter->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTfCenter->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	m_pTfRight = eck::CreateDefTextFormatWithSize(15);
	m_pTfRight->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);

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

	// 左侧选择夹
	m_TabPanel.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	// 标题
	m_LAPageTitle.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_LAPageTitle.SetTextFormat(pTextFormat);
	pTextFormat->Release();
	// 页 主页
	m_PageMain.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_PageMain.SetTextFormat(m_pTfCenter);
	// 页 列表
	m_PageList.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_PageList.SetTextFormat(m_pTfLeft);
	// 页 效果
	m_PageEffect.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_PageEffect.SetTextFormat(m_pTfLeft);
	// 页 设置
	m_PageOptions.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_PageOptions.SetTextFormat(m_pTfLeft);
	// 页 播放
	m_PagePlaying.Create(nullptr, 0, 0,
		0, 0, 0, 0, nullptr, this);
	m_PagePlaying.SetTextFormat(m_pTfLeft);
	// 底部播放控制栏
	m_PlayPanel.Create(nullptr, Dui::DES_VISIBLE /*| Dui::DES_BLURBKG*/, 0,
		0, 0, 0, 0, nullptr, this);
	// 进度条（保证Z序）
	m_TBProgress.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_TBProgress.SetRange(0, 100);
	m_TBProgress.SetPos(50);
	m_TBProgress.SetTrackSize(CyProgressTrack);
	// 标题栏（保证Z序）
	m_TitleBar.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);

	ShowPage(Page::List, FALSE);
	return TRUE;
}

void CWndMain::ShowPage(Page ePage, BOOL bAnimate)
{
	__assume(ePage < Page::Max);
	const int idxShow = (int)ePage;
	const int idxShowLast = (int)m_eCurrPage;
	ClearPageAnimation();
	if (idxShow == idxShowLast)
		return;
	m_eCurrPage = ePage;

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
			return;// 已经显示，不需要动画
		m_bPageAnUpToDown = (idxShow < idxShowLast);
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
					const auto x = p->m_pAnPage->GetRect().left;
					constexpr int yNormal = CyPageTitle + DTopPageTitle + CxPageIntPadding;
					if (p->m_bPageAnUpToDown)
						p->m_pAnPage->SetPos(x, yNormal + 
							CyPageSwitchAnDelta - (int)fCurrValue);
					else
						p->m_pAnPage->SetPos(x, yNormal - 
							CyPageSwitchAnDelta + (int)fCurrValue);
				});
		}
		m_pecPage->SetRange(0, CyPageSwitchAnDelta);
		m_pecPage->Begin();
		WakeRenderThread();
	}
}

void CWndMain::ClearPageAnimation()
{
	if (!m_pAnPage)
		return;
	m_pecPage->End();
	//m_pAnPage->SetStyle(m_pAnPage->GetStyle() & ~Dui::DES_COMPOSITED);
	m_pAnPage = nullptr;
}

void CWndMain::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
		m_TBProgress.SetPos(float(App->GetPlayer().GetCurrTime() * ProgBarScale));
		m_TBProgress.InvalidateRect();
	break;
	case PlayEvt::Play:
		m_TBProgress.SetRange(0.f, float(App->GetPlayer().GetTotalTime() * ProgBarScale));
		m_TBProgress.SetPos(0.f);
		m_TBProgress.InvalidateRect();
		[[fallthrough]];
	case PlayEvt::Resume:
		SetTimer(HWnd, IDT_COMM_TICK, TE_COMM_TICK, nullptr);
		break;
	case PlayEvt::Stop:
		m_TBProgress.SetPos(0.f);
		m_TBProgress.InvalidateRect();
		[[fallthrough]];
	case PlayEvt::Pause:
		KillTimer(HWnd, IDT_COMM_TICK);
		break;
	}
}

CWndMain::~CWndMain()
{
}

LRESULT CWndMain::OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TIMER:
		if (wParam == IDT_COMM_TICK)
		{
			App->GetPlayer().GetSignal().Emit(PLAY_EVT_PARAM{ PlayEvt::CommTick });
		}
		break;
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
			DTopPageTitle,
			CxTabPanel + CxTabToPagePadding + CxPageTitle,
			DTopPageTitle + CyPageTitle });
		for (auto& e : m_vPage)
			e->SetRect({
				CxTabPanel + CxTabToPagePadding,
				CyPageTitle + DTopPageTitle + CxPageIntPadding,
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

	case Dui::TBE_POSCHANGED:
	{
		if (pElem == &m_TBProgress)
		{
			App->GetPlayer().SetPosition(
				m_TBProgress.GetPos() / ProgBarScale);
			return 0;
		}
	}
	break;
	}
	return __super::OnElemEvent(pElem, uMsg, wParam, lParam);
}

IDWriteTextFormat* CWndMain::TfClone()
{
	return eck::CreateDefTextFormatWithSize(15);
}
