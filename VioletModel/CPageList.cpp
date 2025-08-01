#include "pch.h"
#include "CWndMain.h"

constexpr std::wstring_view ColumnName[]
{
	L"名称"sv,
	L"艺术家"sv,
	L"专辑"sv,
	L"时长"sv,
};

eck::CoroTask<void> CPageList::TskLoadSongData(std::shared_ptr<LISTFILE> pListFile,
	std::vector<LOAD_DATA_ITEM> vItem)
{
	struct TMP
	{
		eck::CRefStrW rsTitle;
		eck::CRefStrW rsArtist;
		eck::CRefStrW rsAlbum;
		ComPtr<IWICBitmap> pWicBitmap;
		ComPtr<ID2D1Bitmap1> pD2DBitmap;
		Tag::MUSICPIC Pic;
		UINT uSecTime;
	};
	auto UiThread{ eck::CoroCaptureUiThread(((CWndMain*)GetWnd())->ThreadCtx()) };
	auto Token{ co_await eck::CoroGetPromiseToken() };
	co_await eck::CoroResumeBackground();
	//co_await eck::CoroSleep(1000);
	std::vector<TMP> vTmp(vItem.size());
	EckCounter(vItem.size(), i)
	{
		if (Token.GetPromise().IsCanceled())
			co_return;
		const auto& e = vItem[i];
		auto& f = vTmp[i];
		CBass Bass{};
		auto h = Bass.Open(e.rsFile.Data(), BASS_STREAM_DECODE,
			BASS_STREAM_DECODE, BASS_STREAM_DECODE);
		if (!h)
			EckDbgPrintFmt(L"%s打开失败", e.rsFile.Data());
		const UINT uSecTime = (UINT)round(Bass.GetLength());
		Bass.Close();

		Tag::CMediaFile File{ e.rsFile.Data() };
		File.DetectTag();
		Tag::MUSICINFO mi{};
		mi.uMask = Tag::MIM_ALL;
		mi.uFlag = Tag::MIF_JOIN_ARTIST;
		if (File.GetTagType() & (Tag::TAG_ID3V2_3 | Tag::TAG_ID3V2_4))
		{
			Tag::CID3v2 Id3v2{ File };
			Id3v2.ReadTag(0);
			Id3v2.SimpleExtractMove(mi);
		}
		f.rsTitle = std::move(mi.rsTitle);
		if (mi.Artist.index() == 1u)
			f.rsArtist = mi.GetArtistStr();
		f.rsAlbum = std::move(mi.rsAlbum);
		f.uSecTime = uSecTime;
		const auto pCover = mi.GetMainCover();
		if (pCover)
		{
			f.Pic = std::move(*pCover);
			ComPtr<IStream> pStream;
			if (pCover->bLink)
			{
				SHCreateStreamOnFileEx(std::get<eck::CRefStrW>(f.Pic.varPic).Data(),
					STGM_READ, 0, FALSE, nullptr, &pStream);
			}
			else
				pStream = new eck::CStreamView{ std::get<eck::CRefBin>(f.Pic.varPic) };
			eck::CreateWicBitmap(f.pWicBitmap.RefOf(), pStream.Get(),
				m_cxIl, m_cyIl, eck::DefWicPixelFormat,
				WICBitmapInterpolationModeHighQualityCubic);
			if (f.pWicBitmap.Get())
			{
				D2D1_BITMAP_PROPERTIES1 BmpProp{};
				BmpProp.pixelFormat = D2D1_PIXEL_FORMAT(
					DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
				m_pDC->GetDpi(&BmpProp.dpiX, &BmpProp.dpiY);
				m_pDC->CreateBitmapFromWicBitmap(f.pWicBitmap.Get(),
					BmpProp, &f.pD2DBitmap);
			}
		}
	}
	co_await UiThread;
	if (Token.GetPromise().IsCanceled())
		co_return;
	ECK_DUILOCK;
	for (auto it = m_vLoadDataTask.begin(); it != m_vLoadDataTask.end(); ++it)
		if (&it->Task.GetPromise() == &Token.GetPromise())
		{
			m_vLoadDataTask.erase(it);
			break;
		}
	EckCounter(vItem.size(), i)
	{
		const auto Tag = vItem[i].Tag;
		//EckDbgPrint(Tag);
		auto& f = vTmp[i];
		const auto pItem = pListFile->PlayList.FindTag(Tag);
		if (!pItem)
			co_return;
		if (f.pD2DBitmap.Get())
		{
			const auto idxImg = m_pIlList->AddImage(f.pD2DBitmap.Get());
			if (idxImg >= 0)
				pItem->idxIl = idxImg;
		}
		pItem->rsTitle = std::move(f.rsTitle);
		pItem->rsArtist = std::move(f.rsArtist);
		pItem->rsAlbum = std::move(f.rsAlbum);
		if (!pItem->rsTitle.IsEmpty())
			pItem->rsName = pItem->rsTitle;
		pItem->s.uSecTime = f.uSecTime;
		pItem->s.bUpdated = TRUE;
		pItem->TskTag = 0ull;
		m_GLList.InvalidateCache(vItem[i].idxFlat);
	}
	m_GLList.InvalidateRect();
}

CPlayList* CPageList::GetCurrPlayList()
{
	const auto idx = m_TBLPlayList.GetCurrSel();
	if (idx < 0)
		return nullptr;
	return &m_vListFile[idx]->PlayList;
}

HRESULT CPageList::OnMenuAddFile(std::shared_ptr<LISTFILE> pListFile, int idxInsert)
{
	ComPtr<IFileOpenDialog> pfod;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfod));
	if (FAILED(hr))
		return hr;
	pfod->SetTitle(L"打开音频文件");
	pfod->SetOptions(FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
	constexpr COMDLG_FILTERSPEC FilterSpec[]
	{
		{L"音频文件(*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff)",
			L"*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff"},
		{L"所有文件",L"*.*"}
	};
	pfod->SetFileTypes(ARRAYSIZE(FilterSpec), FilterSpec);

	eck::GetThreadCtx()->bEnableDarkModeHook = FALSE;
	pfod->Show(GetWnd()->HWnd);
	eck::GetThreadCtx()->bEnableDarkModeHook = TRUE;
	ComPtr<IShellItemArray> psia;
	hr = pfod->GetResults(&psia);
	if (FAILED(hr))
		return hr;
	DWORD cItems;
	hr = psia->GetCount(&cItems);
	if (FAILED(hr))
		return hr;
	ComPtr<IShellItem> psi;
	PWSTR pszFile;
	eck::CCsGuard _{ GetCriticalSection() };
	EckCounter(cItems, i)
	{
		psia->GetItemAt(i, psi.AddrOfClear());
		psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFile);
		if (pszFile)
		{
			pListFile->PlayList.FlInsert(pszFile, idxInsert);
			if (idxInsert >= 0)
				++idxInsert;
		}
	}
	return S_OK;
}

void CPageList::OnPlayEvent(const PLAY_EVT_PARAM& e)
{

}

void CPageList::UpdateDefCover()
{
	SafeRelease(m_pBmpDefCover);
	ComPtr<IWICBitmap> pDefCover;
	eck::ScaleWicBitmap(App->GetImg(GImg::DefaultCover), pDefCover.RefOf(),
		m_cxIl, m_cyIl, WICBitmapInterpolationModeHighQualityCubic);
	D2D1_BITMAP_PROPERTIES1 BmpProp{};
	BmpProp.pixelFormat = D2D1_PIXEL_FORMAT(
		DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
	m_pDC->GetDpi(&BmpProp.dpiX, &BmpProp.dpiY);
	m_pDC->CreateBitmapFromWicBitmap(pDefCover.Get(), BmpProp, &m_pBmpDefCover);
}

LRESULT CPageList::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
	{
		if (wParam == (WPARAM)&m_TBLPlayList)
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::TBLE_GETDISPINFO:
			{
				const auto p = (Dui::NMTBLDISPINFO*)lParam;
				if (p->uMask & eck::DIM_TEXT)
					p->cchText = swprintf((PWSTR)p->pszText, L"播放列表 %d", p->idx);
				if (p->uMask & eck::DIM_IMAGE)
					p->pImage = ((CWndMain*)GetWnd())->RealizeImg(GImg::List);
			}
			return 0;
			case Dui::TBLE_SELCHANGED:
			{
				const auto p = (Dui::NMTBLITEMINDEX*)lParam;
				const auto& e = *m_vListFile[p->idx];
				m_GLList.SetItemCount(e.PlayList.FlGetCount());
				m_GLList.ReCalc();
				m_GLList.InvalidateRect();
			}
			return 0;
			}
		else if (wParam == (WPARAM)&m_GLList)
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::LEE_GETDISPINFO:
			{
				const auto p = (Dui::NMLEDISPINFO*)lParam;
				if (p->bItem)
				{
					const auto pList = GetCurrPlayList();
					if (!pList)
						return 0;
					const auto& e = pList->FlAt(p->Item.idx);
					if (p->uMask & eck::DIM_TEXT)
					{
						switch (p->Item.idxSub)
						{
						case 0:
							p->Item.pszText = e.rsName.Data();
							p->Item.cchText = e.rsName.Size();
							break;
						case 1:
							p->Item.pszText = e.rsArtist.Data();
							p->Item.cchText = e.rsArtist.Size();
							break;
						case 2:
							p->Item.pszText = e.rsAlbum.Data();
							p->Item.cchText = e.rsAlbum.Size();
							break;
						case 3:
						{
							m_rsDispInfoBuf.Format(L"%02u:%02u",
								e.s.uSecTime / 60u, e.s.uSecTime % 60);
							p->Item.pszText = m_rsDispInfoBuf.Data();
							p->Item.cchText = m_rsDispInfoBuf.Size();
						}
						break;
						}
					}
					if (p->Item.idxSub)
						p->Item.idxImg = -1;
					else
						p->Item.idxImg = e.idxIl;
				}
			}
			return 0;
			case Dui::HEE_GETDISPINFO:
			{
				const auto p = (Dui::NMHEDISPINFO*)lParam;
				p->pszText = ColumnName[p->idx].data();
				p->cchText = (int)ColumnName[p->idx].size();
			}
			return 0;
			case Dui::LTE_SCROLLED:
			{
				const auto idx = m_TBLPlayList.GetCurrSel();
				if (idx < 0)
					break;
				const auto pListFile = m_vListFile[idx];
				const auto p = (Dui::NMLTSCROLLED*)lParam;
				if (!m_vLoadDataTask.empty())
					for (size_t i{ m_vLoadDataTask.size() - 1 }; i; --i)
					{
						auto& e = m_vLoadDataTask[i];
						if (!(std::max(e.idxFlatBegin, p->idxBegin) <=
							std::min(e.idxFlatEnd, p->idxEnd)))
						{
							e.Task.TryCancel();
							for (int i = e.idxFlatBegin; i <= e.idxFlatEnd; ++i)
								pListFile->PlayList.FlAt(i).TskTag = 0ull;
							m_vLoadDataTask.erase(m_vLoadDataTask.begin() + i);
						}
					}
				std::vector<LOAD_DATA_ITEM> vItem;
				eck::CTimeIdGenerator IdGen{};

				for (int i = p->idxBegin; i <= p->idxEnd; ++i)
				{
					auto& e = pListFile->PlayList.FlAt(i);
					if (e.TskTag || e.s.bUpdated)
						continue;
					e.TskTag = IdGen.Generate();
					auto& f = vItem.emplace_back();
					f.rsFile = e.rsFile;
					f.Tag = e.TskTag;
					f.idxFlat = i;
				}
				if (!vItem.empty())
				{
					auto& e = m_vLoadDataTask.emplace_back();
					e.idxFlatBegin = p->idxBegin;
					e.idxFlatEnd = p->idxEnd;
					e.Task = TskLoadSongData(pListFile, vItem);
				}
			}
			return 0;
			}
		else if (wParam == (WPARAM)&m_BTAddFile)
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::EE_COMMAND:
			{
				const auto idx = m_TBLPlayList.GetCurrSel();
				if (idx < 0)
					break;
				const auto e = m_vListFile[idx];
				OnMenuAddFile(e, -1);
				m_GLList.SetItemCount(e->PlayList.FlGetCount());
				m_GLList.ReCalc();
				m_GLList.InvalidateRect();
			}
			return 0;
			}
	}
	return 0;

	case WM_SIZE:
		m_Lyt.Arrange(0, 0, GetWidth(), GetHeight());
		break;

	case WM_SETFONT:
	{
		m_TBLPlayList.SetTextFormat(GetTextFormat());
		m_EDSearch.SetTextFormat(GetTextFormat());
		m_BTAddFile.SetTextFormat(GetTextFormat());
	}
	return 0;

	case WM_CREATE:
	{
		m_cxIl = Log2Phy(CxyListCover);
		m_cyIl = m_cxIl;
		m_pIlList = new eck::CD2DImageList{ CxyListCover,CxyListCover };
		m_pIlList->BindRenderTarget(m_pDC);
		UpdateDefCover();
		m_pIlList->AddImage(m_pBmpDefCover);
		{
			m_EDSearch.TxSetProp(TXTBIT_MULTILINE, 0, FALSE);
			m_EDSearch.Create(nullptr, Dui::DES_VISIBLE, 0,
				0, 0, 0, CyStdEdit, this);
			m_LytPlayList.Add(&m_EDSearch, { .cyBottomHeight = CxPageIntPadding }, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

			m_TBLPlayList.Create(nullptr, Dui::DES_VISIBLE, 0,
				0, 0, CxListFileList, 0, this, GetWnd());
			m_TBLPlayList.SetItemCount(40);
			m_TBLPlayList.SetBottomExtraSpace(CyPlayPanel);
			m_TBLPlayList.ReCalc();
			m_vListFile.resize(40);
			for (auto& e : m_vListFile)
				e = std::make_shared<LISTFILE>();
			m_LytPlayList.Add(&m_TBLPlayList, {}, eck::LF_FILL, 1);
		}
		m_Lyt.Add(&m_LytPlayList, { .cxRightWidth = CxPageIntPadding },
			eck::LF_FIX_WIDTH | eck::LF_FILL_HEIGHT);

		{
			ComPtr<IDWriteTextFormat> pTextFormat;
			App->GetFontFactory().NewFont(pTextFormat.RefOf(), eck::Align::Near,
				eck::Align::Center, (float)CyFontNormal, 400, TRUE);
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			m_BTAddFile.Create(L"添加文件", Dui::DES_VISIBLE, 0,
				0, 0, 100, 30, this);
			m_LytList.Add(&m_BTAddFile, {}, eck::LF_FIX);
			m_GLList.Create(nullptr, Dui::DES_VISIBLE, 0,
				0, 0, 0, 0, this);
			m_GLList.SetView(Dui::CListTemplate::Type::Report);
			constexpr int Width[]
			{
				160, 150, 150, 50
			};
			m_GLList.SetColumnCount(4, Width);
			m_GLList.SetTopExtraSpace(Dui::CListTemplate::CyDefHeader);
			m_GLList.SetBottomExtraSpace(CyPlayPanel);
			m_GLList.SetItemHeight(CyPlayListItem);
			m_GLList.SetSingleSel(FALSE);
			m_GLList.SetTextFormat(pTextFormat.Get());
			m_GLList.SetImageList(m_pIlList);
			m_GLList.GetHeader().SetTextFormat(pTextFormat.Get());
			m_LytList.Add(&m_GLList, {}, eck::LF_FILL, 1);
		}
		m_Lyt.Add(&m_LytList, { .cxRightWidth = CxPageIntPadding }, eck::LF_FILL, 1);

		m_GLList.GetSignal().Connect(
			[&](UINT uMsg, WPARAM wParam, LPARAM lParam, eck::SlotCtx& Ctx)
			{
				switch (uMsg)
				{
				case WM_LBUTTONDBLCLK:
				{
					POINT pt ECK_GET_PT_LPARAM(lParam);
					m_GLList.ClientToElem(pt);
					Dui::LE_HITTEST ht{ pt };
					const auto idx = m_GLList.HitTest(ht);
					if (idx < 0)
						break;
					App->GetPlayer().SetList(GetCurrPlayList());
					App->GetPlayer().Play(idx);
				}
				break;
				}
				return 0;
			});
	}
	break;
	case WM_DPICHANGED:
	{
		m_cxIl = Log2Phy(CxyListCover);
		m_cyIl = m_cxIl;
		
	}
	break;
	case WM_DESTROY:
	{
		m_vListFile.clear();
		SafeRelease(m_pIlList);
		SafeRelease(m_pBmpDefCover);
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}
