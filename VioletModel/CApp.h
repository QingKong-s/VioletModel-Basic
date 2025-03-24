#pragma once
enum class GPal
{
	TabPanelBk,
	PlayPanelBk,

	Max
};

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

	Max
};

constexpr inline PCWSTR ImgFile[]
{
	LR"(About.png)",
	LR"(AboutBg.png)",
	LR"(AboutLogo.png)",
	LR"(AboutLogo12.png)",
	LR"(Add.png)",
	LR"(ArrowLeft.png)",
	LR"(ArrowLeftW.png)",
	LR"(ArrowRight.png)",
	LR"(ArrowRightW.png)",
	LR"(Aurorast.png)",
	LR"(AurorastDark.png)",
	LR"(BigLogo.png)",
	LR"(Copy.png)",
	LR"(Default_Cover.png)",
	LR"(Delete.png)",
	LR"(File.png)",
	LR"(FileW.png)",
	LR"(Folder.png)",
	LR"(FolderW.png)",
	LR"(FoundryOymyakon.png)",
	LR"(Home.png)",
	LR"(HomeW.png)",
	LR"(License.png)",
	LR"(List.png)",
	LR"(List_PlayList.png)",
	LR"(ListW.png)",
	LR"(Player_Volume_0.png)",
	LR"(Player_Volume_1.png)",
	LR"(Player_Volume_2.png)",
	LR"(Player_Volume_3.png)",
	LR"(Player_Volume_Mute.png)",
	LR"(PlayPage_Down.png)",
	LR"(PlayPage_Up.png)",
	LR"(PlayW.png)",
	LR"(Plugin.png)",
	LR"(PluginW.png)",
	LR"(Settings.png)",
	LR"(SettingsW.png)",
	LR"(SmallLogo.png)",
	LR"(Test.jpg)",
	LR"(WindowLogo.png)",
};

static_assert(ARRAYSIZE(ImgFile) == (size_t)GImg::Max, "ImgFile size error.");

constexpr PCWSTR MainWndPageName[]
{
	L"主页",
	L"列表",
	L"插件",
	L"设置",
};

enum :int
{
	CxTabPanel = 48,
	CyPalyPanel = 100,
	CxyWndLogo = 18,
	CyTitleBar = 40,
	CyPageTitle = 60,
	CxPageIntPadding = 10,
	CxTabToPagePadding = 14,
	CxListFileList = 140,
	CyProgress = 20,
	CyProgressTrack = 12,
};

class CApp
{
private:
	Dui::CThemePalette* m_pThemePalette;

	IWICBitmap* m_Img[ARRAYSIZE(ImgFile)];

	constexpr static D2D1_COLOR_F m_cr[]
	{
		{ 1.f,1.f,1.f,0.6f },
		{ 1.f,1.f,1.f,0.6f },
	};
	constexpr static D2D1_COLOR_F crDark[]
	{
		{ 0.2f,0.2f,0.99f,1.f },
		{ 0.2f,0.2f,0.4f,1.f },
	};
	BOOL m_bDarkMode{};
public:
	CApp();

	static void Init();

	constexpr const D2D1_COLOR_F& GetColor(GPal n) const
	{
		return m_bDarkMode ? crDark[size_t(n)] : m_cr[size_t(n)];
	}

	constexpr IWICBitmap* GetImg(GImg n) const
	{
		return m_Img[size_t(n)];
	}

	EckInlineCe void SetDarkMode(BOOL bDarkMode) { m_bDarkMode = bDarkMode; }
};

extern CApp* App;