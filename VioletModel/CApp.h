#pragma once
#include "CPlayer.h"
#include "CPlayListMgr.h"

enum class GImg
{
	About,
	AboutBg,
	AboutLogo,
	AboutLogo12,
	Add,
	ArrowLeft,
	ArrowLeftW,
	ArrowRight,
	ArrowRightW,
	Aurorast,
	AurorastDark,
	BigLogo,
	Copy,
	DefaultCover,
	Delete,
	File,
	FileW,
	Folder,
	FolderW,
	FoundryOymyakon,
	Home,
	HomeW,
	License,
	List,
	ListPlayList,
	ListW,
	PlayerVolume0,
	PlayerVolume1,
	PlayerVolume2,
	PlayerVolume3,
	PlayerVolumeMute,
	PlayPageDown,
	PlayPageUp,
	PlayW,
	Plugin,
	PluginW,
	Settings,
	SettingsW,
	SmallLogo,
	Test,
	WindowLogo,
	ArrowCross,
	ArrowRight3,
	Circle,
	Next,
	Prev,
	Triangle,
	CircleOne,
	ArrowRight1,
	Lrc,
	Pause,
	NextSolid,
	PrevSolid,
	PauseSolid,
	TriangleSolid,

	Max
};

constexpr PCWSTR MainWndPageName[]
{
	L"主页",
	L"列表",
	L"效果",
	L"设置",
};

enum :int
{
	CxTabPanel = 48,
	CyPlayPanel = 100,
	CxyWndLogo = 18,
	CyTitleBar = 40,
	DTopPageTitle = 12,
	CxPageTitle = 120,
	CyPageTitle = 34,
	CxPageIntPadding = 10,
	CxTabToPagePadding = 14,
	CxListFileList = 170,
	CyProgress = 20,
	CyProgressTrack = 12,
	DLeftMiniCover = 50,
	DTopMiniCover = 12,
	CxyMiniCover = 80,
	CxyPlayPageArrow = 30,
	CxPaddingPlayPanelText = 20,
	DTopTitle = 24,
	CyPlayPanelText = 20,
	CyPaddingTitleAndArtist = 14,
	CxMaxTitleAndArtist = 130,
	DTopTime = 40,
	CxMaxTime = 110,
	CxyCircleButton = 34,
	CxyCircleButtonBig = 44,
	CxPaddingCircleButton = 24,
	CxPaddingCircleButtonRightEdge = 32,
	CxyCircleButtonImage = 18,
	CyPageSwitchAnDelta = 60,
	CxPaddingCtrlBtnWithPlayPage = 44,
	CxPaddingProgBarWithPlayPage = 70,
	CyFontNormal = 15,
	CyFontPageTitle = 20,
	CyStdList = 32,
	CyStdEdit = CyStdList,
	CxyListCover = 40,
	CyPlayListItem = 46,
	CyVolTB = 16,
	CyVolTrack = 10,
	CxVolBar = 220,
	CyVolBar = 32,
	CxVolLabel = 30,
	CxVolBarPadding = 8,
	DVolAn = 10,
	DLrcTop = 50,
	DLrcBottom = 135,
	CxyBackBtn = 40,
	CyPlayPageLabelCoverPadding = 24,
	CyPlayPageLabelPadding = 4,
	CyPlayPageLabel = 26,
	CyFontPlayPageLabel = 18,
};

enum class GPal
{
	TabPanelBk,

	PlayPanelBk,
	PlayPanelBlurMask,
	PlayPanelWatermark,

	VolBarBk,
	VolBarBorder,

	PlayBtnBkNormal,
	PlayBtnBkHot,
	PlayBtnBkSelected,

	PlayPageOverlay,

	ScrollBarThumb,

	LrcTextNormal,
	LrcTextHighlight,

	Max
};

// 所有的ID，包括窗口定时器、WM_COMMAND、控件ID等
enum
{
	VIOLET_ID_BEGIN = 0x514B,

	IDT_COMM_TICK,

	IDTBB_PREV,
	IDTBB_PLAY,
	IDTBB_NEXT,

	ELEID_PLAYPAGE_BACK,
	ELEID_VOLBAR_TRACK,

	TE_COMM_TICK = 300,
};

enum
{
	ELEN_PLACEHOLDER = Dui::EE_PRIVATE_BEGIN,
	ELEN_PAGE_CHANGE,		// [CTabPanel]边栏被单击时(NMLTITEMINDEX*)
	ELEN_MINICOVER_CLICK,	// [CMiniCover]封面被单击时
};

class CApp
{
private:
	IWICBitmap* m_Img[(size_t)GImg::Max];
	BOOL m_bDarkMode{};
	CPlayer m_Player{};
	CPlayListMgr m_ListMgr{};
	eck::CDWriteFontFactory m_FontFactory{};
	eck::THREADCTX* m_ptcUiThread{};
public:
	CApp();

	~CApp();

	static void Init();

	const D2D1_COLOR_F& GetColor(GPal n) const;

	EckInlineNdCe IWICBitmap* GetImg(GImg n) const { return m_Img[size_t(n)]; }

	EckInlineCe void SetDarkMode(BOOL bDarkMode)
	{
		if (m_bDarkMode != bDarkMode)
			m_bDarkMode = bDarkMode;
	}

	EckInlineNdCe auto& GetPlayer() { return m_Player; }
	EckInlineNdCe auto& GetListMgr() { return m_ListMgr; }
	EckInlineNdCe auto& GetFontFactory() { return m_FontFactory; }

	EckInlineNdCe auto UiThreadCtx() const { return m_ptcUiThread; }
};

extern CApp* App;