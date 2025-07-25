#include "pch.h"

#include "CWndMain.h"

void CWndMain::ClearRes()
{
	for (auto& e : m_vBmpRealization)
		SafeRelease(e);
	SafeRelease(m_pecPage);
	SafeRelease(m_pBmpCover);
	SafeRelease(m_pBrush);
	SafeRelease(m_pCompPlayPageAn);
	SafeRelease(m_pCompNormalPageAn);
}

BOOL CWndMain::OnCreate(HWND hWnd, CREATESTRUCT* pcs)
{
	m_WndTbGhost.SetText(pcs->lpszName);

	m_ptcUiThread = eck::GetThreadCtx();
	CBass::Init();
	App->GetPlayer().GetSignal().Connect(this, &CWndMain::OnPlayEvent);
	m_pCompPlayPageAn = new CCompPlayPageAn{};
	m_pCompNormalPageAn = new Dui::CCompositorPageAn{};
	m_pCompNormalPageAn->SetWorkDC(GetDeviceContext());
	m_pCompNormalPageAn->InitAsScaleBlur();
	m_pCompNormalPageAn->InitAsScaleOpacity();
	GetDeviceContext()->CreateSolidColorBrush({}, &m_pBrush);
	RegisterTimeLine(this);

	eck::GetThreadCtx()->UpdateDefColor();
	MARGINS m{};// 不能使用-1，否则会绘制标准标题栏
	m.cxLeftWidth = 65536 * 4;
	DwmExtendFrameIntoClientArea(hWnd, &m);
	eck::EnableWindowMica(hWnd);

	SmtcInit();

	BlurInit();
	BlurSetUseLayer(TRUE);
	ComPtr<IDWriteTextFormat> pTfPageTitle, pTfLeft, pTfCenter;
	App->GetFontFactory().NewFont(pTfPageTitle.RefOf(), eck::Align::Near,
		eck::Align::Center, (float)CyFontPageTitle, 600);
	App->GetFontFactory().NewFont(pTfLeft.RefOf(), eck::Align::Near,
		eck::Align::Center, (float)CyFontNormal);
	App->GetFontFactory().NewFont(pTfCenter.RefOf(), eck::Align::Center,
		eck::Align::Center, (float)CyFontNormal);

	m_NormalPageContainer.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	const auto pNormalParent = &m_NormalPageContainer;
	// 左侧选择夹
	m_TabPanel.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_TO_WND, 0,
		0, 0, 0, 0, pNormalParent, this);
	// 标题
	m_LAPageTitle.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, pNormalParent, this);
	m_LAPageTitle.SetTextFormat(pTfPageTitle.Get());
	// 页 主页
	m_PageMain.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, pNormalParent, this);
	m_PageMain.SetTextFormat(pTfCenter.Get());
	// 页 列表
	m_PageList.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, pNormalParent, this);
	m_PageList.SetTextFormat(pTfLeft.Get());
	// 页 效果
	m_PageEffect.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, pNormalParent, this);
	m_PageEffect.SetTextFormat(pTfLeft.Get());
	// 页 设置
	m_PageOptions.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, pNormalParent, this);
	m_PageOptions.SetTextFormat(pTfLeft.Get());
	// 底部播放控制栏
	m_PlayPanel.Create(nullptr, Dui::DES_VISIBLE/* | Dui::DES_BLURBKG*/, 0,
		0, 0, 0, 0, nullptr, this);
	m_PlayPanel.SetTextFormat(pTfLeft.Get());
	// 页 播放
	m_PagePlaying.Create(nullptr, 0, 0,
		0, 0, 0, 0, nullptr, this);
	m_PagePlaying.SetTextFormat(pTfLeft.Get());
	// 进度条
	m_TBProgress.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_TBProgress.SetRange(0, 100);
	m_TBProgress.SetTrackPos(50);
	m_TBProgress.SetTrackSize(CyProgressTrack);
	// 按钮 上一曲
	m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTPrev.SetImage(RealizeImg(GImg::Prev));
	m_BTPrev.SetCustomDraw(TRUE);
	m_BTPrev.SetTransparentBk(TRUE);
	// 按钮 播放/暂停
	m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButtonBig, CxyCircleButtonBig, nullptr, this);
	m_BTPlay.SetImage(RealizeImg(GImg::Triangle));
	m_BTPlay.SetCustomDraw(TRUE);
	// 按钮 下一曲
	m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTNext.SetImage(RealizeImg(GImg::Next));
	m_BTNext.SetCustomDraw(TRUE);
	m_BTNext.SetTransparentBk(TRUE);
	// 按钮 歌词
	m_BTLrc.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTLrc.SetImage(RealizeImg(GImg::Lrc));
	m_BTLrc.SetCustomDraw(TRUE);
	m_BTLrc.SetTransparentBk(TRUE);
	// 按钮 音量
	m_BTVol.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTVol.SetImage(RealizeImg(GImg::PlayerVolume3));
	m_BTVol.SetCustomDraw(TRUE);
	m_BTVol.SetTransparentBk(TRUE);
	// 标题栏
	m_TitleBar.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	UpdateButtonImageSize();
	m_PagePlaying.UpdateBlurredCover();
	OnCoverUpdate();

	StUpdateColorizationColor();
	StSwitchStdThemeMode(ShouldAppsUseDarkMode());
	App->SetDarkMode(ShouldAppsUseDarkMode());
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
			m_pecPage->SetAnProc(eck::Easing::OutExpo);
			m_pecPage->SetCallBack([](float fCurrValue, float fOldValue, LPARAM lParam)
				{
					const auto p = (CWndMain*)lParam;
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
	m_pAnPage = nullptr;
}

CWndMain::~CWndMain()
{
}

void CWndMain::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
	{
		m_TBProgress.SetTrackPos(float(App->GetPlayer().GetCurrTime() * ProgBarScale));
		m_TBProgress.InvalidateRect();
		SmtcOnCommonTick();
	}
	break;
	case PlayEvt::Play:
	{
		m_PagePlaying.UpdateBlurredCover();
		OnCoverUpdate();
		SmtcUpdateTimeLineRange();
		SmtcUpdateDisplay();
		if (m_PagePlaying.GetStyle() & Dui::DES_VISIBLE)
			m_PagePlaying.InvalidateRect();
		m_TBProgress.SetRange(0.f, float(App->GetPlayer().GetTotalTime() * ProgBarScale));
		m_TBProgress.SetTrackPos(0.f);
		m_TBProgress.InvalidateRect();
		m_PlayPanel.InvalidateRect();
		m_WndTbGhost.InvalidateThumbnailCache();
		m_WndTbGhost.SetIconicThumbnail();
	}
	[[fallthrough]];
	case PlayEvt::Resume:
	{
		SetTimer(HWnd, IDT_COMM_TICK, TE_COMM_TICK, nullptr);
		m_BTPlay.SetImage(RealizeImg(GImg::Pause));
		m_BTPlay.InvalidateRect();
		TblUpdatePalyPauseButtonIcon(FALSE);
	}
	break;
	case PlayEvt::Stop:
	{
		m_TBProgress.SetTrackPos(0.f);
		m_TBProgress.InvalidateRect();
	}
	[[fallthrough]];
	case PlayEvt::Pause:
	{
		KillTimer(HWnd, IDT_COMM_TICK);
		m_BTPlay.SetImage(RealizeImg(GImg::Triangle));
		m_BTPlay.InvalidateRect();
		TblUpdatePalyPauseButtonIcon(TRUE);
	}
	break;
	}
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
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);
		ClearPageAnimation();
		const auto cxClient = GetClientWidthLog();
		const auto cyClient = GetClientHeightLog();
		m_pCompNormalPageAn->RefPoint = { cxClient / 2.f,cyClient / 2.f };
		m_NormalPageContainer.SetRect({ 0,0,cxClient,cyClient });
		m_TitleBar.SetRect({ 0,0,cxClient,CyTitleBar });
		m_TabPanel.SetRect({ 0,0,CxTabPanel,cyClient - CyPlayPanel });

		const auto yPlayPanel = cyClient - CyPlayPanel;
		m_PlayPanel.SetRect({ 0,cyClient - CyPlayPanel,cxClient,cyClient });

		m_LAPageTitle.SetRect({
			CxTabPanel + CxTabToPagePadding,
			DTopPageTitle,
			CxTabPanel + CxTabToPagePadding + CxPageTitle,
			DTopPageTitle + CyPageTitle });

		m_PagePlaying.SetRect({ 0,0,cxClient,cyClient });
		m_rcPPLarge = { 0.f,0.f,(float)cxClient,(float)cyClient };
		m_rcPPMini.left = DLeftMiniCover;
		m_rcPPMini.top = float(cyClient - CyPlayPanel + DTopMiniCover);
		m_rcPPMini.right = m_rcPPMini.left + (float)CxyMiniCover;
		m_rcPPMini.bottom = m_rcPPMini.top + (float)CxyMiniCover;
		for (auto& e : m_vPage)
			e->SetRect({
				CxTabPanel + CxTabToPagePadding,
				CyPageTitle + DTopPageTitle + CxPageIntPadding,
				cxClient,
				cyClient - CxTabToPagePadding });
		if (!m_bPPAnActive)
			RePosButtonProgBar();
		OnCoverUpdate();
		return lResult;
	}

	case WM_NCCALCSIZE:
	{
		const auto cxFrame = eck::DaGetSystemMetrics(SM_CXFRAME, GetDpiValue());
		const auto cyFrame = eck::DaGetSystemMetrics(SM_CYFRAME, GetDpiValue());
		const auto cxPadded = eck::DaGetSystemMetrics(SM_CXPADDEDBORDER, GetDpiValue());
		return eck::MsgOnNcCalcSize(wParam, lParam,
			{ cxFrame + cxPadded,cxFrame + cxPadded,0,cyFrame + cxPadded });
	}
	break;

	case WM_COMMAND:
		if (TblOnCommand(wParam))
			return 0;
		break;

	case WM_CREATE:
		__super::OnMsg(hWnd, uMsg, wParam, lParam);
		return HANDLE_WM_CREATE(hWnd, wParam, lParam, OnCreate);
	case WM_DESTROY:
		KillTimer(hWnd, IDT_COMM_TICK);
		__super::OnMsg(hWnd, uMsg, wParam, lParam);
		m_WndTbGhost.Destroy();
		SmtcUnInit();
		ClearRes();
		PostQuitMessage(0);
		return 0;
	case WM_SYSCOLORCHANGE:
		eck::MsgOnSysColorChangeMainWnd(hWnd, wParam, lParam);
		break;
	case WM_SETTINGCHANGE:
	{
		if (eck::MsgOnSettingChangeMainWnd(hWnd, wParam, lParam, TRUE))
		{
			const auto bDark = ShouldAppsUseDarkMode();
			App->SetDarkMode(bDark);
			StSwitchStdThemeMode(bDark);
			Redraw();
		}
	}
	break;
	case WM_DWMCOLORIZATIONCOLORCHANGED:
		StUpdateColorizationColor();
		break;
	case WM_SETTEXT:
	{
		const auto lResult = __super::OnMsg(hWnd, uMsg, wParam, lParam);
		if (lResult)
			m_WndTbGhost.SetText((PCWSTR)lParam);
		return lResult;
	}
	break;
	}
	return __super::OnMsg(hWnd, uMsg, wParam, lParam);
}

LRESULT CWndMain::OnElemEvent(Dui::CElem* pElem, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case ELEN_PAGE_CHANGE:
	{
		const auto* const p = (Dui::NMLTITEMINDEX*)lParam;
		ShowPage((Page)p->idx, TRUE);
	}
	return 0;

	case Dui::TBE_POSCHANGED:
	{
		if (pElem == &m_TBProgress)
		{
			App->GetPlayer().SetPosition(
				m_TBProgress.GetTrackPos() / ProgBarScale);
			return 0;
		}
	}
	return 0;

	case ELEN_MINICOVER_CLICK:
	{
		ECK_DUILOCK;
		PreparePlayPageAnimation();
		m_PagePlaying.SetVisible(TRUE);
		WakeRenderThread();
	}
	return 0;

	case Dui::EE_COMMAND:
	{
		if (pElem == &m_BTPlay)
			App->GetPlayer().PlayOrPause();
		else if (pElem == &m_BTPrev)
			App->GetPlayer().Prev();
		else if (pElem == &m_BTNext)
			App->GetPlayer().Next();
		else if (pElem->GetID() == ELEID_PLAYPAGE_BACK)
		{
			ECK_DUILOCK;
			PreparePlayPageAnimation();
			WakeRenderThread();
		}
	}
	break;
	}
	return __super::OnElemEvent(pElem, uMsg, wParam, lParam);
}

void CWndMain::Tick(int iMs)
{
	constexpr float Duration[]
	{
		MaxPPAnDuration,
		MaxPPAnDuration * 5 / 6,
		MaxPPAnDuration * 5 / 6,
		MaxPPAnDuration * 4 / 6,
	};
	constexpr float DurationR[]
	{
		MaxPPAnDuration * 4 / 6,
		MaxPPAnDuration * 8 / 9,
		MaxPPAnDuration * 5 / 6,
		MaxPPAnDuration,
	};
	m_msPPTotalTime += iMs;
	if (m_msPPTotalTime >= MaxPPAnDuration || m_msPPTotalTime <= 0.f)
	{
		m_msPPTotalTime = 0.f;
		m_bPPAnActive = FALSE;
		m_PagePlaying.SetCompositor(nullptr);
		m_NormalPageContainer.SetCompositor(nullptr);
		m_NormalPageContainer.SetStyle(m_NormalPageContainer.GetStyle() &
			~Dui::DES_BASE_BEGIN_END_PAINT);
		if (m_bPPAnReverse)
		{
			m_NormalPageContainer.SetVisible(FALSE);
			m_PlayPanel.SetVisible(FALSE);
		}
		else
			m_PagePlaying.SetVisible(FALSE);
		return;
	}
	float k;
	m_kPalyPageAn = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, MaxPPAnDuration);
	if (!m_bPPAnReverse)
		m_kPalyPageAn = 1.f - m_kPalyPageAn;
	// 移动底部的按钮
	RePosButtonProgBar();
	// 页面动画更新
	m_pCompPlayPageAn->Opacity = m_kPalyPageAn;
	m_pCompNormalPageAn->Scale = 1.f - (m_kPalyPageAn * 0.2f);
	m_pCompNormalPageAn->Update2DAffineTransform();
	m_pCompNormalPageAn->SetBlurStdDeviation(20.f * m_kPalyPageAn);
	m_pCompNormalPageAn->Opacity = 1.f - m_kPalyPageAn;
	m_pCompNormalPageAn->UpdateOpacity();

	D2D1_POINT_2F pt[4];
	if (m_bPPAnReverse)
	{
		// 左上
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, DurationR[0]);
		k = 1.f - k;
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.left, m_rcPPLarge.top,
			m_rcPPMini.left, m_rcPPMini.top, k, pt[0].x, pt[0].y);
		// 右上
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, DurationR[1]);
		k = 1.f - k;
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.right, m_rcPPLarge.top,
			m_rcPPMini.right, m_rcPPMini.top, k, pt[1].x, pt[1].y);
		// 左下
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, DurationR[2]);
		k = 1.f - k;
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.left, m_rcPPLarge.bottom,
			m_rcPPMini.left, m_rcPPMini.bottom, k, pt[2].x, pt[2].y);
		// 右下
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, DurationR[3]);
		k = 1.f - k;
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.right, m_rcPPLarge.bottom,
			m_rcPPMini.right, m_rcPPMini.bottom, k, pt[3].x, pt[3].y);
	}
	else
	{
		// 左上
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, Duration[0]);
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.left, m_rcPPLarge.top,
			m_rcPPMini.left, m_rcPPMini.top, k, pt[0].x, pt[0].y);
		// 右上
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, Duration[1]);
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.right, m_rcPPLarge.top,
			m_rcPPMini.right, m_rcPPMini.top, k, pt[1].x, pt[1].y);
		// 左下
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, Duration[2]);
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.left, m_rcPPLarge.bottom,
			m_rcPPMini.left, m_rcPPMini.bottom, k, pt[2].x, pt[2].y);
		// 右下
		k = eck::Easing::OutExpo(m_msPPTotalTime, 0.f, 1.f, Duration[3]);
		k = std::clamp(k, 0.f, 1.f);
		eck::CalcPointFromLineScalePos(m_rcPPLarge.right, m_rcPPLarge.bottom,
			m_rcPPMini.right, m_rcPPMini.bottom, k, pt[3].x, pt[3].y);
	}
	m_pCompPlayPageAn->Mat = eck::CalcDistortMatrix(m_rcPPLarge, pt);
	m_pCompPlayPageAn->MatR = eck::CalcInverseDistortMatrix(m_rcPPLarge, pt);
	if (!m_PagePlaying.GetCompositor())
		m_PagePlaying.SetCompositor(m_pCompPlayPageAn);
	if (!m_NormalPageContainer.GetCompositor())
	{
		m_NormalPageContainer.SetCompositor(m_pCompNormalPageAn);
		m_NormalPageContainer.SetStyle(Dui::DES_BASE_BEGIN_END_PAINT |
			m_NormalPageContainer.GetStyle());
	}
	m_PagePlaying.CompReCalcCompositedRect();
	m_NormalPageContainer.CompReCalcCompositedRect();

	Redraw();
}

void CWndMain::UpdateButtonImageSize()
{
	constexpr D2D1_SIZE_F Size{ CxyCircleButtonImage, CxyCircleButtonImage };
	m_BTPrev.SetImageSize(Size);
	m_BTPlay.SetImageSize(Size);
	m_BTNext.SetImageSize(Size);
	m_BTLrc.SetImageSize(Size);
	m_BTVol.SetImageSize(Size);
}

void CWndMain::PreparePlayPageAnimation()
{
	ECKBOOLNOT(m_bPPAnReverse);
	if (!m_bPPAnActive)
	{
		m_msPPTotalTime = m_bPPAnReverse ? MaxPPAnDuration : 0.f;
		m_msPPTotalTime = 0.f;
	}
	m_bPPAnActive = TRUE;
	m_PlayPanel.m_Cover.SetVisible(!m_bPPAnReverse);
	if (!m_bPPAnReverse)
	{
		m_NormalPageContainer.SetVisible(TRUE);
		m_PlayPanel.SetVisible(TRUE);
	}
	WakeRenderThread();
}

void CWndMain::RePosButtonProgBar()
{
	const auto cxClient = GetClientWidthLog();
	const auto cyClient = GetClientHeightLog();
	// 移动右侧按钮
	const auto y = cyClient - CyPlayPanel + (CyPlayPanel - CxyCircleButton) / 2;
	int x = cxClient - CxyCircleButton - CxPaddingCircleButtonRightEdge;
	x += int((x - (cxClient - CxPaddingCtrlBtnWithPlayPage)) * m_kPalyPageAn);

	m_BTVol.SetPos(x, y);
	x -= (CxyCircleButton + CxPaddingCircleButton);

	m_BTLrc.SetPos(x, y);
	x -= (CxyCircleButton + CxPaddingCircleButton);

	x = XCenterButtonLeftLimit + ((x - XCenterButtonLeftLimit) -
		(CxyCircleButton * 2 + CxyCircleButtonBig + CxPaddingCircleButton * 2)) / 2;
	const auto xCenter = (cxClient - (CxyCircleButton * 2 + CxyCircleButtonBig +
		CxPaddingCircleButton * 2)) / 2;
	x += int((xCenter - x) * m_kPalyPageAn);
	// 移动中间按钮
	m_BTPrev.SetPos(x, y);
	x += (CxyCircleButton + CxPaddingCircleButton);

	m_BTPlay.SetPos(x, cyClient - CyPlayPanel + (CyPlayPanel - CxyCircleButtonBig) / 2);
	x += (CxyCircleButtonBig + CxPaddingCircleButton);

	m_BTNext.SetPos(x, y);

	//-----移动进度条
	const auto yPlayPanel = cyClient - CyPlayPanel;
	const auto dTrackSpacing = m_TBProgress.GetTrackSpacing();
	const auto oxIndent = int((float)CxPaddingProgBarWithPlayPage * m_kPalyPageAn);
	m_TBProgress.SetRect({
		-(int)dTrackSpacing + oxIndent,
		yPlayPanel - CyProgress / 2,
		cxClient + (int)dTrackSpacing - oxIndent,
		yPlayPanel + CyProgress / 2 });
}

void CWndMain::OnCoverUpdate()
{
	const auto pBmp = m_PagePlaying.m_pBmpCover;
	if (pBmp)
	{
		m_pCompPlayPageAn->SetOverlayBitmap(pBmp);
		m_PlayPanel.m_Cover.SetBitmap(pBmp);
	}
}