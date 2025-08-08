#include "pch.h"

#include "CWndMain.h"

#include "eck\Env.h"

#include "CPlayList.h"

#if defined _WIN64
#pragma comment(lib, R"(Bass\bass_x64.lib)")
#pragma comment(lib, R"(Bass\bass_fx_x64.lib)")
#pragma comment(lib, R"(Bass\bassmidi_x64.lib)")
#elif defined _WIN32
#pragma comment(lib, R"(Bass\bass.lib)")
#pragma comment(lib, R"(Bass\bass_fx.lib)")
#pragma comment(lib, R"(Bass\bassmidi.lib)")
#endif
#pragma comment(lib, "RuntimeObject.lib")

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pszCmdLine, _In_ int nCmdShow)
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(1003);

	if (const auto hr = CoInitialize(nullptr); FAILED(hr))
	{
		EckDbgPrintFormatMessage(hr);
		eck::MsgBox(eck::Format(L"CoInitialize failed!\r\nhr = %08X", hr),
			L"Error", MB_ICONERROR);
		return 0;
	}

	DWORD dwErr;
	if (const auto r = eck::Init(hInstance, nullptr, &dwErr);
		r != eck::InitStatus::Ok)
	{
		EckDbgPrintFormatMessage(dwErr);
		eck::MsgBox(eck::Format(LR"(Init failed!
InitStatus = %d
ErrorCode = %08X
Msg = %s)",
(int)r, dwErr, eck::InitStatusToString(r)), L"Error", MB_ICONERROR);
		return 0;
	}

	App = new CApp{};
	CApp::Init();
//#ifdef _DEBUG
	App->GetListMgr().Add()->SetName(L"测试列表"sv);
//#endif

	const auto pWnd = new CWndMain{};
	const auto hMon = eck::GetOwnerMonitor(nullptr);
	const auto iDpi = eck::GetMonitorDpi(hMon);
	auto size = SIZE{ 910,620 };
	eck::DpiScale(size, iDpi);
	const auto pt = eck::CalcCenterWndPos(nullptr, size.cx, size.cy, FALSE);
	pWnd->SetUserDpi(iDpi);
	pWnd->SetPresentMode(Dui::PresentMode::DCompositionSurface);
	pWnd->SetTransparent(TRUE);
	//pWnd->SetDrawDirtyRect(1);
	pWnd->Create(L"示例Win32程序", WS_POPUP | WS_VISIBLE | WS_CAPTION |
		WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX, 0,
		pt.x, pt.y, size.cx, size.cy, nullptr, 0);
	pWnd->Visible = TRUE;

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		if (!eck::PreTranslateMessage(msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	delete pWnd;
	delete App;
	eck::ThreadUnInit();
	eck::UnInit();
	CoUninitialize();
#ifdef _DEBUG
	if (eck::g_pDxgiDebug)
		eck::g_pDxgiDebug->ReportLiveObjects(
			DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
#endif
	return (int)msg.wParam;
}