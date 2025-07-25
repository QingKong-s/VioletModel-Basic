#include "pch.h"
#include "CWndMain.h"

static HRESULT ScaleImageForButton(GImg eImg, int iDpi,
	_Out_ IWICBitmap*& pBmp)
{
	const auto cxy = eck::DpiScale(20, iDpi);
	return eck::ScaleWicBitmap(App->GetImg(eImg), pBmp,
		cxy, cxy, WICBitmapInterpolationModeFant);
}

HRESULT CWndMain::TblCreateGhostWindow()
{
	m_WndTbGhost.Create(nullptr, WS_OVERLAPPEDWINDOW,
		WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		-32000, -32000, 0, 0, nullptr, nullptr);
	return S_OK;
}

HRESULT CWndMain::TblSetup()
{
	m_pTaskbarList->UnregisterTab(m_WndTbGhost.HWnd);
	m_pTaskbarList->RegisterTab(m_WndTbGhost.HWnd, HWnd);
#pragma warning(suppress: 6387)// 可能为NULL
	m_pTaskbarList->SetTabOrder(m_WndTbGhost.HWnd, nullptr);

	HICON hiPrev, hiNext;
	IWICBitmap* pBmp;
	ScaleImageForButton(GImg::PrevSolid, GetDpiValue(), pBmp);
	hiPrev = eck::CreateHICON(pBmp);
	pBmp->Release();
	ScaleImageForButton(GImg::TriangleSolid, GetDpiValue(), pBmp);
	m_hiTbPlay.reset(eck::CreateHICON(pBmp));
	pBmp->Release();
	ScaleImageForButton(GImg::PauseSolid, GetDpiValue(), pBmp);
	m_hiTbPause.reset(eck::CreateHICON(pBmp));
	pBmp->Release();
	ScaleImageForButton(GImg::NextSolid, GetDpiValue(), pBmp);
	hiNext = eck::CreateHICON(pBmp);
	pBmp->Release();

	THUMBBUTTON tb[3]{};
	constexpr auto dwMask = THB_ICON | THB_TOOLTIP;
	tb[0].dwMask = dwMask;
	tb[0].hIcon = hiPrev;
	tb[0].iId = IDTBB_PREV;
	eck::TcsCopyLen(tb[0].szTip, EckArrAndLen(L"上一曲"));

	tb[1].dwMask = dwMask;
	tb[1].hIcon = m_hiTbPlay.get();
	tb[1].iId = IDTBB_PLAY;
	eck::TcsCopyLen(tb[1].szTip, EckArrAndLen(L"播放"));

	tb[2].dwMask = dwMask;
	tb[2].hIcon = hiNext;
	tb[2].iId = IDTBB_NEXT;
	eck::TcsCopyLen(tb[2].szTip, EckArrAndLen(L"下一曲"));

	const auto hr = m_pTaskbarList->ThumbBarAddButtons(
		m_WndTbGhost.HWnd, ARRAYSIZE(tb), tb);
	DestroyIcon(hiNext);
	DestroyIcon(hiPrev);
	return S_OK;
}

HRESULT CWndMain::TblUpdateToolBarIcon()
{
	HICON hiPrev, hiNext;
	IWICBitmap* pBmp;
	ScaleImageForButton(GImg::PrevSolid, GetDpiValue(), pBmp);
	hiPrev = eck::CreateHICON(pBmp);
	pBmp->Release();
	ScaleImageForButton(GImg::TriangleSolid, GetDpiValue(), pBmp);
	m_hiTbPlay.reset(eck::CreateHICON(pBmp));
	pBmp->Release();
	ScaleImageForButton(GImg::PauseSolid, GetDpiValue(), pBmp);
	m_hiTbPause.reset(eck::CreateHICON(pBmp));
	pBmp->Release();
	ScaleImageForButton(GImg::NextSolid, GetDpiValue(), pBmp);
	hiNext = eck::CreateHICON(pBmp);
	pBmp->Release();

	THUMBBUTTON tb[3]{};
	constexpr auto dwMask = THB_ICON;
	tb[0].dwMask = dwMask;
	tb[0].hIcon = hiPrev;
	tb[0].iId = IDTBB_PREV;

	tb[1].dwMask = dwMask;
	tb[1].hIcon = m_hiTbPlay.get();
	tb[1].iId = IDTBB_PLAY;

	tb[2].dwMask = dwMask;
	tb[2].hIcon = hiNext;
	tb[2].iId = IDTBB_NEXT;

	m_pTaskbarList->ThumbBarUpdateButtons(m_WndTbGhost.HWnd, ARRAYSIZE(tb), tb);
	DestroyIcon(hiNext);
	DestroyIcon(hiPrev);
	return S_OK;
}

HRESULT CWndMain::TblCreateObjectAndInit()
{
	if (m_pTaskbarList.Get())
		return S_FALSE;
	m_pTaskbarList.CreateInstance(CLSID_TaskbarList);
	return m_pTaskbarList->HrInit();
}

BOOL CWndMain::TblOnCommand(WPARAM wParam)
{
	if (HIWORD(wParam) != THBN_CLICKED)
		return FALSE;
	switch (LOWORD(wParam))
	{
	case IDTBB_NEXT:
		App->GetPlayer().Next();
		return TRUE;
	case IDTBB_PLAY:
		App->GetPlayer().PlayOrPause();
		return TRUE;
	case IDTBB_PREV:
		App->GetPlayer().Prev();
		return TRUE;
	}
	return FALSE;
}

HRESULT CWndMain::TblUpdatePalyPauseButtonIcon(BOOL bPlay)
{
	THUMBBUTTON tb{};
	tb.dwMask = THB_ICON;
	tb.iId = IDTBB_PLAY;
	tb.hIcon = bPlay ? m_hiTbPlay.get() : m_hiTbPause.get();
	return m_pTaskbarList->ThumbBarUpdateButtons(m_WndTbGhost.HWnd, 1, &tb);
}

HRESULT CWndMain::SmtcInit() noexcept
{
#if VIOLET_WINRT
	HRESULT hr;
	//////////TMPTMPTMP//////////
	PWSTR pszProgramPath;
	hr = SHGetKnownFolderPath(FOLDERID_CommonPrograms, 0, nullptr, &pszProgramPath);
	if (SUCCEEDED(hr))
	{
		eck::CRefStrW rsLink{ pszProgramPath };
		rsLink.PushBack(EckStrAndLen(L"\\VioletModel.lnk"));
		if (!PathFileExistsW(rsLink.Data()))
			eck::CreateShortcut(rsLink.Data(),
				NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer);
		CoTaskMemFree(pszProgramPath);
	}
	//////////TMPTMPTMP//////////
	try
	{
		auto pSmtcInterop = winrt::get_activation_factory<
			WinMedia::SystemMediaTransportControls,
			ISystemMediaTransportControlsInterop>();
		hr = pSmtcInterop->GetForWindow(HWnd,
			winrt::guid_of<WinMedia::SystemMediaTransportControls>(),
			winrt::put_abi(m_Smtc));
		if (FAILED(hr)) return hr;
	}
	catch (winrt::hresult_error& e)
	{
		return e.to_abi();
	}

	m_Smtc.IsEnabled(true);
	m_Smtc.IsPlayEnabled(true);
	m_Smtc.IsPauseEnabled(true);
	m_Smtc.IsNextEnabled(true);
	m_Smtc.IsPreviousEnabled(true);

	m_SmtcEvtTokenButtonPressed = m_Smtc.ButtonPressed(
		[&](const WinMedia::SystemMediaTransportControls&,
			const WinMedia::SystemMediaTransportControlsButtonPressedEventArgs& Args)
		{
			const auto eBtn = Args.Button();
			m_ptcUiThread->Callback.EnQueueCallback([=, this]
				{
					switch (eBtn)
					{
					case WinMedia::SystemMediaTransportControlsButton::Play:
						App->GetPlayer().PlayOrPause(FALSE);
						break;
					case WinMedia::SystemMediaTransportControlsButton::Pause:
						App->GetPlayer().PlayOrPause(TRUE);
						break;
					case WinMedia::SystemMediaTransportControlsButton::Next:
						App->GetPlayer().Next();
						break;
					case WinMedia::SystemMediaTransportControlsButton::Previous:
						App->GetPlayer().Prev();
						break;
					default:
						return;
					}
					SmtcUpdateState();
					SmtcUpdateTimeLinePosition();
				});
		});
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

#if VIOLET_WINRT
eck::CoroTask<> CWndMain::SmtcCoroUpdateDisplay()
{
	const auto mi = App->GetPlayer().GetMusicInfo();// 复制一份
	auto Token{ co_await eck::CoroGetPromiseToken() };
	co_await eck::CoroResumeBackground();

	auto u = m_Smtc.DisplayUpdater();
	u.ClearAll();
	u.Type(WinMedia::MediaPlaybackType::Music);
	auto MusicProp = u.MusicProperties();
	MusicProp.Artist(mi.GetArtistStr().Data());
	MusicProp.Title(mi.rsTitle.Data());
	MusicProp.AlbumTitle(mi.rsAlbum.Data());
	if (!mi.rsGenre.IsEmpty())
	{
		auto Genres = MusicProp.Genres();
		Genres.Append(mi.rsGenre.Data());
	}
	const auto pCover = mi.GetMainCover();
	if (pCover)
	{
		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Storage::Streams;
		auto Stream = u.Thumbnail();
		if (pCover->bLink)
		{
			auto Task{ StorageFile::GetFileFromPathAsync(
				std::get<eck::CRefStrW>(pCover->varPic).Data()) };
			Token.GetPromise().SetCanceller([](void* p)
				{
					((decltype(Task)*)p)->Cancel();
				}, &Task);
			auto File{ co_await Task };
			Token.GetPromise().SetCanceller(nullptr, nullptr);
			u.Thumbnail(RandomAccessStreamReference::CreateFromFile(File));
		}
		else
		{
			InMemoryRandomAccessStream InMemStream;
			DataWriter w{ InMemStream };
			const auto& rb = std::get<eck::CRefBin>(pCover->varPic);
			w.WriteBytes(winrt::array_view<const uint8_t>{ rb.Data(), (uint32_t)rb.Size() });
			auto Task{ w.StoreAsync() };
			Token.GetPromise().SetCanceller([](void* p)
				{
					((decltype(Task)*)p)->Cancel();
				}, &Task);
			co_await Task;
			Token.GetPromise().SetCanceller(nullptr, nullptr);
			w.DetachStream();
			u.Thumbnail(RandomAccessStreamReference::CreateFromStream(InMemStream));
		}
	}
	u.Update();
}
#endif// VIOLET_WINRT

HRESULT CWndMain::SmtcUpdateDisplay() noexcept
{
#if VIOLET_WINRT
	if (m_TskSmtcUpdateDisplay.hCoroutine &&
		!m_TskSmtcUpdateDisplay.IsCompleted())
	{
		m_TskSmtcUpdateDisplay.TryCancel();
		m_TskSmtcUpdateDisplay.SyncWait();
	}
	m_TskSmtcUpdateDisplay = SmtcCoroUpdateDisplay();
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWndMain::SmtcOnCommonTick() noexcept
{
#if VIOLET_WINRT
	const auto ullTick = NtGetTickCount64();
	if (ullTick - m_ullSmtcTimeLineLastUpdate >= 5000)
	{
		m_ullSmtcTimeLineLastUpdate = ullTick;
		return SmtcUpdateTimeLinePosition();
	}
	else
		return S_FALSE;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWndMain::SmtcUpdateTimeLineRange() noexcept
{
#if VIOLET_WINRT
	using winrt::Windows::Foundation::TimeSpan;
	m_SmtcTimeline.StartTime(TimeSpan{});
	m_SmtcTimeline.MinSeekTime(TimeSpan{});

	const auto lfSeconds = App->GetPlayer().GetTotalTime();
	const TimeSpan Dur{ std::chrono::milliseconds{ LONGLONG(lfSeconds * 1000.) } };
	m_SmtcTimeline.EndTime(Dur);
	m_SmtcTimeline.MaxSeekTime(Dur);
	m_Smtc.UpdateTimelineProperties(m_SmtcTimeline);
	m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Playing);
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWndMain::SmtcUpdateTimeLinePosition() noexcept
{
#if VIOLET_WINRT
	using winrt::Windows::Foundation::TimeSpan;
	const auto lfSeconds = App->GetPlayer().GetCurrTime();
	const TimeSpan Pos{ std::chrono::milliseconds{ LONGLONG(lfSeconds * 1000.) } };
	m_SmtcTimeline.Position(Pos);
	m_Smtc.UpdateTimelineProperties(m_SmtcTimeline);
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWndMain::SmtcUpdateState() noexcept
{
#if VIOLET_WINRT
	const auto& Player = App->GetPlayer();
	if (!Player.IsActive())
	{
		m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Closed);
		return S_OK;
	}
	if (Player.IsPaused())
		m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Paused);
	else
		m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Playing);
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

void CWndMain::SmtcUnInit() noexcept
{
#if VIOLET_WINRT
	m_Smtc.ButtonPressed(m_SmtcEvtTokenButtonPressed);
#endif// VIOLET_WINRT
}