#pragma once
#include "CApp.h"
#include "CPage.h"
#include "CPageMain.h"
#include "CPageList.h"
#include "CPageEffect.h"
#include "CPageOptions.h"
#include "CPagePlaying.h"
#include "CTabPanel.h"
#include "CPlayPanel.h"
#include "CCompPlayPageAn.h"
#include "CWndTbGhost.h"

class CWndMain final :
	public Dui::CDuiWnd,
	public eck::CFixedTimeLine
{
	friend class CWndTbGhost;
public:
	enum class Page : BYTE
	{
		Main,
		List,
		Effect,
		Options,
		Max
	};

	enum
	{
		// 使用普通窗口定时器驱动进度和歌词轮询即可
		IDT_COMM_TICK = 0x514B,
		TE_COMM_TICK = 300,
	};

	enum
	{
		IDTBB_PREV = 10001,
		IDTBB_PLAY,
		IDTBB_NEXT
	};

	constexpr static double ProgBarScale = 100.;
private:
	constexpr static float MaxPPAnDuration = 700.f;
	constexpr static int XCenterButtonLeftLimit = DLeftMiniCover + CxyMiniCover +
		CxPaddingPlayPanelText + CxMaxTitleAndArtist + CxPaddingPlayPanelText +
		CxMaxTime + CxPaddingPlayPanelText;
	Dui::CTitleBar m_TitleBar{};
	Dui::CLabel m_LAPageTitle{};

	CTabPanel m_TabPanel{};
	CPlayPanel m_PlayPanel{};

	CPageMain m_PageMain{};
	CPageList m_PageList{};
	CPageEffect m_PageEffect{};
	CPageOptions m_PageOptions{};
	Dui::CElem m_NormalPageContainer{};// 所有下层页面的父级，动画时用

	CPagePlaying m_PagePlaying{};

	Dui::CTrackBar m_TBProgress{};

	Dui::CCircleButton
		m_BTPrev{},
		m_BTPlay{},
		m_BTNext{},
		m_BTLrc{},
		m_BTVol{};

	ID2D1Bitmap1* m_vBmpRealization[(size_t)GImg::Max]{};
	ID2D1SolidColorBrush* m_pBrush{};

	ID2D1Bitmap1* m_pBmpCover{};

	CPage* m_vPage[(size_t)Page::Max]{ &m_PageMain,&m_PageList,&m_PageEffect,&m_PageOptions };

	CPage* m_pAnPage{};
	eck::CEasingCurve* m_pecPage{};

	BOOLEAN m_bPageAnUpToDown{};
	Page m_eCurrPage{};

	BOOLEAN m_bPPAnActive{};
	BOOLEAN m_bPPAnReverse{};// TRUE = 小到大，FALSE = 大到小
	float m_msPPTotalTime{};
	float m_kPalyPageAn{};
	D2D1_RECT_F m_rcPPMini{};
	D2D1_RECT_F m_rcPPLarge{};
	CCompPlayPageAn* m_pCompPlayPageAn{};

	Dui::CCompositorPageAn* m_pCompNormalPageAn{};

	eck::THREADCTX* m_ptcUiThread{};

	eck::UniquePtr<eck::DelHIcon>
		m_hiTbPlay{},
		m_hiTbPause{};
	ComPtr<ITaskbarList4> m_pTaskbarList{};
	CWndTbGhost m_WndTbGhost{ *this };

#if VIOLET_WINRT
	WinMedia::SystemMediaTransportControls m_Smtc{ nullptr };
	eck::CoroTask<> m_TskSmtcUpdateDisplay{};
	WinMedia::SystemMediaTransportControlsTimelineProperties m_SmtcTimeline{};
	ULONGLONG m_ullSmtcTimeLineLastUpdate{};
#endif
private:
	void ClearRes();

	BOOL OnCreate(HWND hWnd, CREATESTRUCT* pcs);

	void ShowPage(Page ePage, BOOL bAnimate);

	void ClearPageAnimation();

	void OnPlayEvent(const PLAY_EVT_PARAM& e);

	void UpdateButtonImageSize();

	void PreparePlayPageAnimation();

	void RePosButtonProgBar();

	void OnCoverUpdate();

	HRESULT TblCreateGhostWindow();
	HRESULT TblSetup();
	HRESULT TblUpdateToolBarIcon();
	HRESULT TblCreateObjectAndInit();
	BOOL TblOnCommand(WPARAM wParam);
	HRESULT TblUpdatePalyPauseButtonIcon(BOOL bPlay);

	HRESULT SmtcInit() noexcept;
#if VIOLET_WINRT
	eck::CoroTask<> SmtcCoroUpdateDisplay();
#endif
	HRESULT SmtcUpdateDisplay() noexcept;
	HRESULT SmtcUpdateTimeLineRange() noexcept;
	HRESULT SmtcUpdateTimeLinePosition() noexcept;
public:
	~CWndMain();

	HWND Create(PCWSTR pszText, DWORD dwStyle, DWORD dwExStyle,
		int x, int y, int cx, int cy, HWND hParent, HMENU hMenu, PCVOID pData = nullptr) override
	{
		TblCreateGhostWindow();

		const auto hWnd = __super::Create(pszText, dwStyle, dwExStyle,
			x, y, cx, cy, hParent, hMenu, pData);

		TblCreateObjectAndInit();
		TblSetup();
		m_WndTbGhost.SetIconicThumbnail();
		return hWnd;
	}

	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	BOOL PreTranslateMessage(const MSG& Msg) override;

	LRESULT OnElemEvent(Dui::CElem* pElem, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	ID2D1Bitmap1* RealizeImg(GImg n)
	{
		if (!m_vBmpRealization[(size_t)n])
		{
			GetDeviceContext()->CreateBitmapFromWicBitmap(App->GetImg(n),
				(const D2D1_BITMAP_PROPERTIES1*)nullptr,
				&m_vBmpRealization[(size_t)n]);
		}
		return m_vBmpRealization[(size_t)n];
	}

	//***ITimeLine***
	void Tick(int iMs);
	BOOL IsValid() { return m_bPPAnActive; }

	EckInlineNdCe auto ThreadCtx() const noexcept { return m_ptcUiThread; }
};