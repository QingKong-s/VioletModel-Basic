#include "pch.h"
#include "CApp.h"

CApp* App{};

CApp::CApp()
{
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
