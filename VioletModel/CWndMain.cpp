#include "pch.h"

#include "CWndMain.h"

EckInlineNdCe GImg AutoNextModeToGImg(AutoNextMode eMode) noexcept
{
	switch (eMode)
	{
	case AutoNextMode::ListLoop: return GImg::Circle;
	case AutoNextMode::List: return GImg::ArrowRight3;
	case AutoNextMode::Radom: return GImg::ArrowCross;
	case AutoNextMode::SingleLoop: return GImg::CircleOne;
	case AutoNextMode::Single: return GImg::ArrowRight1;
	}
	ECK_UNREACHABLE;
}

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
	pTfPageTitle->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	App->GetFontFactory().NewFont(pTfLeft.RefOf(), eck::Align::Near,
		eck::Align::Center, (float)CyFontNormal);
	pTfLeft->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	App->GetFontFactory().NewFont(pTfCenter.RefOf(), eck::Align::Center,
		eck::Align::Center, (float)CyFontNormal);
	pTfCenter->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

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
	ComPtr<IDWriteTextFormat> pTfPP;
	m_PagePlaying.Create(nullptr, 0, 0,
		0, 0, 0, 0, nullptr, this);
	m_PagePlaying.SetTextFormat(pTfLeft.Get());
	App->GetFontFactory().NewFont(pTfPP.RefOfClear(), eck::Align::Near,
		eck::Align::Center, (float)CyFontPlayPageLabel, 600);
	pTfPP->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	m_PagePlaying.SetLabelTextFormatTitle(pTfPP.Get());
	App->GetFontFactory().NewFont(pTfPP.RefOfClear(), eck::Align::Near,
		eck::Align::Center, (float)CyFontPlayPageLabel);
	pTfPP->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	m_PagePlaying.SetLabelTextFormat(pTfPP.Get());
	// 进度条
	m_TBProgress.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	m_TBProgress.SetRange(0, 100);
	m_TBProgress.SetTrackPos(50);
	m_TBProgress.SetTrackSize(CyProgressTrack);
	// 按钮 上一曲
	m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTPrev.SetImage(RealizeImage(GImg::Prev));
	m_BTPrev.SetCustomDraw(TRUE);
	m_BTPrev.SetTransparentBk(TRUE);
	// 按钮 播放/暂停
	m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButtonBig, CxyCircleButtonBig, nullptr, this);
	m_BTPlay.SetImage(RealizeImage(GImg::Triangle));
	m_BTPlay.SetCustomDraw(TRUE);
	// 按钮 下一曲
	m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTNext.SetImage(RealizeImage(GImg::Next));
	m_BTNext.SetCustomDraw(TRUE);
	m_BTNext.SetTransparentBk(TRUE);
	// 按钮 播放模式
	m_BTAutoNext.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTAutoNext.SetImage(RealizeImage(
		AutoNextModeToGImg(App->GetPlayer().GetAutoNextMode())));
	m_BTAutoNext.SetCustomDraw(TRUE);
	m_BTAutoNext.SetTransparentBk(TRUE);
	// 按钮 歌词
	m_BTLrc.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTLrc.SetImage(RealizeImage(GImg::Lrc));
	m_BTLrc.SetCustomDraw(TRUE);
	m_BTLrc.SetTransparentBk(TRUE);
	// 按钮 音量
	m_BTVol.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
	m_BTVol.SetImage(RealizeImage(GImg::PlayerVolume3));
	m_BTVol.SetCustomDraw(TRUE);
	m_BTVol.SetTransparentBk(TRUE);
	// 标题栏
	m_TitleBar.Create(nullptr, Dui::DES_VISIBLE, 0,
		0, 0, 0, 0, nullptr, this);
	// 音量条
	m_VolBar.Create(nullptr, 0, 0,
		0, 0, CxVolBar, CyVolBar, nullptr, this);
	m_VolBar.SetTextFormat(pTfCenter.Get());
	//
	UpdateButtonImageSize();
	m_PagePlaying.UpdateBlurredCover();
	OnCoverUpdate();

	StUpdateColorizationColor();
	StSwitchStdThemeMode(ShouldAppsUseDarkMode());
	App->SetDarkMode(ShouldAppsUseDarkMode());
	ShowPage(Page::List, FALSE);
	m_TabPanel.GetTabList().SelectItemForClick(1);
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
			m_pecPage->SetDuration(250);
			m_pecPage->SetAnProc(eck::Easing::OutCubic);
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
		m_pecPage->Begin(0, (float)CyPageSwitchAnDelta);
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

HWND CWndMain::Create(PCWSTR pszText, DWORD dwStyle, DWORD dwExStyle,
	int x, int y, int cx, int cy, HWND hParent, HMENU hMenu, PCVOID pData)
{
	TblCreateGhostWindow(pszText);

	const auto hWnd = __super::Create(pszText, dwStyle, dwExStyle,
		x, y, cx, cy, hParent, hMenu, pData);

	TblCreateObjectAndInit();
	TblSetup();
	m_WndTbGhost.SetIconicThumbnail();
	return hWnd;
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
		TblUpdateProgress();
	}
	break;
	case PlayEvt::Play:
	{
		OnCoverUpdate();
		SmtcUpdateTimeLineRange();
		SmtcUpdateDisplay();
		TblUpdateProgress();
		if (m_PagePlaying.GetStyle() & Dui::DES_VISIBLE)
			m_PagePlaying.InvalidateRect();
		m_TBProgress.SetRange(0.f, float(App->GetPlayer().GetTotalTime() * ProgBarScale));
		m_TBProgress.SetTrackPos(0.f);
		m_TBProgress.InvalidateRect();
		m_PlayPanel.InvalidateRect();
		m_WndTbGhost.InvalidateThumbnailCache();
		m_WndTbGhost.InvalidateDwmThumbnail();
		m_WndTbGhost.SetIconicThumbnail();
	}
	[[fallthrough]];
	case PlayEvt::Resume:
	{
		SetTimer(HWnd, IDT_COMM_TICK, TE_COMM_TICK, nullptr);
		m_BTPlay.SetImage(RealizeImage(GImg::Pause));
		m_BTPlay.InvalidateRect();
		TblUpdateState();
		SmtcUpdateState();
	}
	break;
	case PlayEvt::Stop:
	{
		m_TBProgress.SetTrackPos(0.f);
		m_TBProgress.InvalidateRect();
		TblUpdateProgress();
	}
	[[fallthrough]];
	case PlayEvt::Pause:
	{
		KillTimer(HWnd, IDT_COMM_TICK);
		m_BTPlay.SetImage(RealizeImage(GImg::Triangle));
		m_BTPlay.InvalidateRect();
		TblUpdateState();
		SmtcUpdateState();
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
			App->GetPlayer().GetSignal().Emit({ PlayEvt::CommTick });
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
	case WM_DPICHANGED:
		SetUserDpi(LOWORD(wParam));
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
		else if (pElem->GetID() == ELEID_VOLBAR_TRACK)
		{
			const auto f = ((Dui::CTrackBar*)pElem)->GetTrackPos();
			App->GetPlayer().GetBass().SetVolume(f / 100.f);
			m_VolBar.OnVolChanged(f);
		}
	}
	return 0;

	case ELEN_MINICOVER_CLICK:
	{
		ECK_DUILOCK;
		PreparePlayPageAnimation();
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
		else if (pElem == &m_BTAutoNext)
		{
			const auto r = App->GetPlayer().NextAutoNextMode();
			m_BTAutoNext.SetImage(RealizeImage(AutoNextModeToGImg(r)));
			m_BTAutoNext.InvalidateRect();
		}
		else if (pElem == &m_BTVol)
		{
			const auto x = GetClientWidthLog() - CxVolBar - CxVolBarPadding;
			const auto y = m_BTVol.GetRectInClient().top - CyVolBar;
			m_VolBar.SetPos(x, y);
			m_VolBar.ShowAnimation();
		}
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

ID2D1Bitmap1* CWndMain::RealizeImage(GImg n)
{
	if (!m_vBmpRealization[(size_t)n])
	{
		GetDeviceContext()->CreateBitmapFromWicBitmap(App->GetImg(n),
			(const D2D1_BITMAP_PROPERTIES1*)nullptr,
			&m_vBmpRealization[(size_t)n]);
	}
	return m_vBmpRealization[(size_t)n];
}

void CWndMain::Tick(int iMs)
{
	constexpr float MaxPPAnDuration = 700.f;
	constexpr float DurOverlayOpacity = 150.f;
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
	if (!(m_bPPAnActive = m_PlayPageAn.Tick((float)iMs, MaxPPAnDuration)))
	{
		m_bPPAnActive = FALSE;
		ZeroMemory(m_bPPCornerAnActive, sizeof(m_bPPCornerAnActive));
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
	// 移动底部的按钮
	RePosButtonProgBar();
	// 页面动画更新
	const auto kOverlay = std::clamp(m_PlayPageAn.Time / DurOverlayOpacity, 0.f, 1.f);
	m_pCompPlayPageAn->Opacity = (m_bPPAnReverse ? (1.f - kOverlay) : kOverlay);
	m_pCompNormalPageAn->Scale = 1.f - m_PlayPageAn.K * 0.2f;
	m_pCompNormalPageAn->Update2DAffineTransform();
	m_pCompNormalPageAn->SetBlurStdDeviation(20.f * m_PlayPageAn.K);
	m_pCompNormalPageAn->Opacity = 1.f - m_PlayPageAn.K;
	m_pCompNormalPageAn->UpdateOpacity();

	D2D1_POINT_2F pt[]
	{
		{ m_rcPPMini.left, m_rcPPMini.top },
		{ m_rcPPMini.right, m_rcPPMini.top },
		{ m_rcPPMini.left, m_rcPPMini.bottom },
		{ m_rcPPMini.right, m_rcPPMini.bottom },
	};
	const D2D1_POINT_2F ptLarge[]
	{
		{ m_rcPPLarge.left, m_rcPPLarge.top },
		{ m_rcPPLarge.right, m_rcPPLarge.top },
		{ m_rcPPLarge.left, m_rcPPLarge.bottom },
		{ m_rcPPLarge.right, m_rcPPLarge.bottom },
	};
	const auto pDur = m_bPPAnReverse ? DurationR : Duration;
	EckCounter(4, i)
	{
		m_bPPCornerAnActive[i] = m_PPCornerAn[i].Tick((float)iMs, pDur[i]);
		eck::CalcPointFromLineScalePos(pt[i].x, pt[i].y,
			ptLarge[i].x, ptLarge[i].y, m_PPCornerAn[i].K, pt[i].x, pt[i].y);
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
	const auto kBegin = m_bPPAnReverse ? 0.f : 1.f;
	const auto kEnd = m_bPPAnReverse ? 1.f : 0.f;
	m_PlayPageAn.Start(kBegin, kEnd, m_bPPAnActive);
	EckCounter(4, i)
	{
		m_PPCornerAn[i].Start(kBegin, kEnd, m_bPPCornerAnActive[i]);
		m_bPPCornerAnActive[i] = TRUE;
	}
	m_bPPAnActive = TRUE;
	if (!m_bPPAnReverse)
	{
		m_NormalPageContainer.SetVisible(TRUE);
		m_PlayPanel.m_Cover.SetVisible(TRUE);
		m_PlayPanel.SetVisible(TRUE);
	}
	m_PagePlaying.SetVisible(TRUE);
	WakeRenderThread();
}

void CWndMain::RePosButtonProgBar()
{
	constexpr int XCenterButtonLeftLimit = DLeftMiniCover + CxyMiniCover +
		CxPaddingPlayPanelText + CxMaxTitleAndArtist + CxPaddingPlayPanelText +
		CxMaxTime + CxPaddingPlayPanelText;
	const auto cxClient = GetClientWidthLog();
	const auto cyClient = GetClientHeightLog();
	// 移动右侧按钮
	const auto y = cyClient - CyPlayPanel + (CyPlayPanel - CxyCircleButton) / 2;
	int x = cxClient - CxyCircleButton - CxPaddingCircleButtonRightEdge;
	x += int((x - (cxClient - CxPaddingCtrlBtnWithPlayPage)) * m_PlayPageAn.K);

	m_BTVol.SetPos(x, y);
	x -= (CxyCircleButton + CxPaddingCircleButton);
	m_BTLrc.SetPos(x, y);
	x -= (CxyCircleButton + CxPaddingCircleButton);
	m_BTAutoNext.SetPos(x, y);
	// 移动中间按钮
	x = XCenterButtonLeftLimit + ((x - XCenterButtonLeftLimit) -
		(CxyCircleButton * 2 + CxyCircleButtonBig + CxPaddingCircleButton * 2)) / 2;
	const auto xCenter = (cxClient - (CxyCircleButton * 2 + CxyCircleButtonBig +
		CxPaddingCircleButton * 2)) / 2;
	x += int((xCenter - x) * m_PlayPageAn.K);
	m_BTPrev.SetPos(x, y);
	x += (CxyCircleButton + CxPaddingCircleButton);
	m_BTPlay.SetPos(x, cyClient - CyPlayPanel + (CyPlayPanel - CxyCircleButtonBig) / 2);
	x += (CxyCircleButtonBig + CxPaddingCircleButton);
	m_BTNext.SetPos(x, y);
	// 移动进度条
	const auto yPlayPanel = cyClient - CyPlayPanel;
	const auto dTrackSpacing = m_TBProgress.GetTrackSpacing();
	const auto oxIndent = int((float)CxPaddingProgBarWithPlayPage * m_PlayPageAn.K);
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