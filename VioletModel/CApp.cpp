#include "pch.h"
#include "CApp.h"

constexpr PCWSTR ImgFile[]
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
	LR"(ArrowCross.png)",
	LR"(ArrowRight3.png)",
	LR"(Circle.png)",
	LR"(Next.png)",
	LR"(Prev.png)",
	LR"(Triangle.png)",
	LR"(CircleOne.png)",
	LR"(ArrowRight1.png)",
	LR"(Lrc.png)",
	LR"(Pause.png)",
	LR"(NextSolid.png)",
	LR"(PrevSolid.png)",
	LR"(PauseSolid.png)",
	LR"(TriangleSolid.png)",
};

static_assert(ARRAYSIZE(ImgFile) == (size_t)GImg::Max, "ImgFile size error.");

constexpr static D2D1_COLOR_F PalLight[]
{ 
	Dui::StMakeBackgroundColorLight(0.5f),
	Dui::StMakeBackgroundColorLight(0.5f),
	Dui::StMakeBackgroundColorLight(0.3f),
	Dui::StMakeBackgroundColorLight(0.3f),
	Dui::StMakeBackgroundColorLight(0.5f),
	Dui::StMakeForegroundColorLight(0.2f),
};
constexpr static D2D1_COLOR_F PalDark[]
{
	Dui::StMakeBackgroundColorDark(0.5f),
	Dui::StMakeBackgroundColorDark(0.5f),
	Dui::StMakeBackgroundColorDark(0.3f),
	Dui::StMakeBackgroundColorDark(0.3f),
	Dui::StMakeBackgroundColorDark(0.5f),
	Dui::StMakeForegroundColorDark(0.2f),
};


CApp* App{};

CApp::CApp()
{
	m_ptcUiThread = eck::GetThreadCtx();
	EckAssert(m_ptcUiThread);

	m_ListMgr.LoadList();

	auto rsPath{ eck::GetRunningPath() };
	rsPath.PushBack(LR"(\Skin\)");
	const auto pszFileName = rsPath.PushBack(48);
	for (size_t i{}; const auto e : ImgFile)
	{
		wcscpy(pszFileName, e);
		if (FAILED(eck::CreateWicBitmap(m_Img[i], rsPath.Data())))
		{
			eck::MsgBox(eck::Format(L"缺少资源文件：%s", e).Data());
			abort();
		}
		++i;
	}
}

CApp::~CApp()
{
	for (auto& e : m_Img)
		SafeRelease(e);
}

void CApp::Init()
{
}

const D2D1_COLOR_F& CApp::GetColor(GPal n) const
{
	return m_bDarkMode ? PalDark[size_t(n)] : PalLight[size_t(n)];
}