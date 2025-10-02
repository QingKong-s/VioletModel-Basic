#include "pch.h"
#include "CWndMain.h"
#include "Utils.h"

constexpr std::wstring_view ColumnName[]
{
    L"名称"sv,
    L"艺术家"sv,
    L"专辑"sv,
    L"时长"sv,
};

eck::CoroTask<void> CPageList::TskLoadSongData(TSKPARAM_LOAD_META_DATA&& Param_)
{
    const auto Param{ std::move(Param_) };
    const auto pList = Param.pList.get();
    struct TMP
    {
        Tag::MUSICINFO mi{};
        ComPtr<IWICBitmap> pWicBitmap{};
        ComPtr<ID2D1Bitmap1> pD2DBitmap{};
        UINT uSecTime{};
    };
    auto UiThread{ eck::CoroCaptureUiThread() };

    std::vector<TMP> vTmp(Param.vItem.size());
    pList->TskIncRef();
    EckCounter(Param.vItem.size(), i)
    {
        auto& e = pList->FlAtAbs(Param.vItem[i]);
        vTmp[i].mi.uMask = Tag::MIM_NONE;
        if (!e.s.bUpdated)
        {
            vTmp[i].mi.uMask |= Tag::MIM_TITLE | Tag::MIM_ARTIST |
                Tag::MIM_ALBUM;
            e.s.bUpdated = TRUE;
        }
        if (!e.s.bCoverUpdated)
        {
            vTmp[i].mi.uMask |= Tag::MIM_COVER;
            e.s.bCoverUpdated = TRUE;
        }
    }
    auto ptc = eck::GetThreadCtx();
    EckAssert(eck::GetThreadCtx() == App->UiThreadCtx());

    co_await eck::CoroResumeBackground();
    Tag::SIMPLE_OPT Opt{};
    Opt.svArtistDiv = Opt.svCommDiv = {};
    EckCounter(Param.vItem.size(), i)
    {
        const auto idxFlat = Param.vItem[i];
        const auto& e = pList->FlAtAbs(idxFlat);
        auto& f = vTmp[i];
        UINT uSecTime;
        if (f.mi.uMask == Tag::MIM_COVER)
            uSecTime = 0;
        else
        {
            CBass Bass{};
            const auto h = Bass.Open(e.rsFile.Data(), BASS_STREAM_DECODE,
                BASS_STREAM_DECODE, BASS_STREAM_DECODE);
#ifdef _DEBUG
            if (!h)
                EckDbgPrintFmt(L"%s打开失败", e.rsFile.Data());
#endif
            uSecTime = h ? (UINT)round(Bass.GetLength()) : 0u;
            Bass.Close();
        }

        VltGetMusicInfo(e.rsFile.Data(), f.mi, Opt);

        f.uSecTime = uSecTime;
        const auto pCover = (Tag::MUSICPIC*)f.mi.GetMainCover();
        if (pCover)
        {
            ComPtr<IStream> pStream;
            pCover->CreateStream(pStream.RefOf());
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
    ECK_DUILOCK;

    EckCounter(Param.vItem.size(), i)
    {
        auto& e = pList->FlAtAbs(Param.vItem[i]);
        auto& f = vTmp[i];
        if (f.pD2DBitmap.Get())
        {
            const auto idxImg = Param.pIl->AddImage(f.pD2DBitmap.Get());
            if (idxImg >= 0)
                e.idxIl = idxImg;
        }
        if (f.mi.uMask != Tag::MIM_COVER)
        {
            e.rsTitle = std::move(f.mi.rsTitle);
            e.rsArtist = std::move(f.mi.slArtist.Str);
            if (!e.rsArtist.IsEmpty())
                e.rsArtist.Erase(0);
            e.rsAlbum = std::move(f.mi.rsAlbum);
            if (f.mi.uMaskChecked & Tag::MIM_TITLE)
                e.rsName = e.rsTitle;
            e.s.uSecTime = f.uSecTime;
            e.s.bUpdated = TRUE;
        }
    }
    for (int i = Param.idxBeginDisplay; i <= Param.idxEndDisplay; ++i)
        if (i < m_GLList.GetItemCount())
            m_GLList.InvalidateCache(i);
    m_GLList.RedrawItem(Param.idxBeginDisplay, Param.idxEndDisplay);
    pList->TskDecRef();
}

CPlayList* CPageList::GetCurrPlayList()
{
    const auto idx = m_TBLPlayList.GetCurrSel();
    if (idx < 0)
        return nullptr;
    return App->GetListMgr().AtList(idx).get();
}

std::shared_ptr<CPlayList> CPageList::GetCurrPlayListShared()
{
    const auto idx = m_TBLPlayList.GetCurrSel();
    if (idx < 0)
        return {};
    return App->GetListMgr().AtList(idx);
}

HRESULT CPageList::OnMenuAddFile(CPlayList* pList, int idxInsert)
{
    ComPtr<IFileOpenDialog> pfod;
    HRESULT hr = pfod.CreateInstance(CLSID_FileOpenDialog);
    if (FAILED(hr))
        return hr;
    pfod->SetTitle(L"打开音频文件");
    pfod->SetOptions(FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    constexpr COMDLG_FILTERSPEC FilterSpec[]
    {
        { L"音频文件(*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff)",
            L"*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff" },
        { L"所有文件",L"*.*" }
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
    ECK_DUILOCK;
    EckCounter(cItems, i)
    {
        psia->GetItemAt(i, psi.AddrOfClear());
        psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFile);
        if (pszFile)
        {
            pList->FlInsert(pszFile, idxInsert);
            if (idxInsert >= 0)
                ++idxInsert;
            CoTaskMemFree(pszFile);
        }
    }
    return S_OK;
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

void CPageList::ReCreateImageList(int idx, BOOL bForce)
{
    if (idx < 0)
        return;
    auto& e = App->GetListMgr().At(idx);
    if (e.pImageList.Get() && !bForce)
        return;
    e.pImageList.Attach(new eck::CD2DImageList{ CxyListCover,CxyListCover });
    e.pImageList->BindRenderTarget(m_pDC);
    e.pImageList->AddImage(m_pBmpDefCover);
}

void CPageList::LoadMetaData(int idxBegin, int idxEnd, int idxList)
{
    if (idxList < 0)
        idxList = m_TBLPlayList.GetCurrSel();
    if (idxList < 0)
        return;
    const auto& e = App->GetListMgr().At(idxList);
    TSKPARAM_LOAD_META_DATA Param
    {
        .pList = e.pList,
        .idxBeginDisplay = idxBegin,
        .idxEndDisplay = idxEnd,
        .pIl = e.pImageList
    };
    for (int i = idxBegin; i <= idxEnd; ++i)
    {
        auto& f = e.pList->FlAt(i);
        if (f.s.bUpdated && f.s.bCoverUpdated)
            continue;
        if (e.pList->FlSchIsActive())
            Param.vItem.emplace_back(e.pList->FlSchAt(i));
        else
            Param.vItem.emplace_back(i);
    }
    if (!Param.vItem.empty())
        TskLoadSongData(std::move(Param));
}

void CPageList::CheckVisibleItemMetaData(int idxList)
{
    int idx0, idx1;
    m_GLList.GetVisibleRange(idx0, idx1);
    LoadMetaData(idx0, idx1, idxList);
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
                {
                    const auto pList = App->GetListMgr().AtList(p->idx);
                    const auto& rsName = pList->GetName();
                    p->cchText = rsName.CopyTo((PWSTR)p->pszText, p->cchText);
                }
                if (p->uMask & eck::DIM_IMAGE)
                    p->pImage = ((CWndMain*)GetWnd())->RealizeImage(GImg::List);
            }
            return 0;
            case Dui::TBLE_SELCHANGED:
            {
                const auto* const p = (Dui::NMTBLITEMINDEX*)lParam;
                if (p->idx < 0)
                    break;
                ReCreateImageList(p->idx, FALSE);
                const auto& e = App->GetListMgr().At(p->idx);
                e.pList->ImEnsureLoaded();
                m_GLList.InvalidateCache();
                m_GLList.SetItemCount(e.pList->FlGetCount());
                m_GLList.SetImageList(e.pImageList.Get());
                m_GLList.ReCalc();
                m_GLList.InvalidateRect();
                CheckVisibleItemMetaData(p->idx);
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
                const auto p = (Dui::NMLTSCROLLED*)lParam;
                if (eck::GetThreadCtx() != App->UiThreadCtx())
                    App->UiThreadCtx()->Callback.EnQueueCallback(
                        [this, idx0 = p->idxBegin, idx1 = p->idxEnd]
                        {
                            LoadMetaData(idx0, idx1);
                        });
                else
                    LoadMetaData(p->idxBegin, p->idxEnd);
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_BTAddFile)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::EE_COMMAND:
            {
                const auto pList = GetCurrPlayList();
                if (!pList)
                    break;
                OnMenuAddFile(pList, -1);
                m_GLList.SetItemCount(pList->FlGetCount());
                m_GLList.ReCalc();
                m_GLList.InvalidateRect();
                CheckVisibleItemMetaData(-1);
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_BTLocate)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::EE_COMMAND:
            {
                const auto pList = GetCurrPlayList();
                if (!pList)
                    break;
                m_GLList.EnsureVisible(TRUE, pList->PlyGetCurrentItem());
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_EDSearchItem)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::EDE_TXNOTIFY:
            {
                const auto p = (Dui::NMEDTXNOTIFY*)lParam;
                if (p->iNotify != EN_CHANGE)
                    break;
                const auto pList = GetCurrPlayListShared();
                if (!pList)
                    break;
                GETTEXTLENGTHEX  gtl{};
                gtl.codepage = eck::CP_UTF16LE;
                gtl.flags = GTL_DEFAULT;
                GETTEXTEX gte{};
                if (gte.cb = m_EDSearchItem.GetTextLengthEx(&gtl))
                {
                    gte.codepage = eck::CP_UTF16LE;
                    gte.flags = GT_DEFAULT;
                    eck::CRefStrW rsFilter{};
                    rsFilter.ReSize((int)gte.cb);
                    gte.cb = (DWORD)rsFilter.ByteSize();

                    m_EDSearchItem.GetTextEx(&gte, rsFilter.Data());

                    pList->FlSchDoSearch(rsFilter.ToStringView());
                    m_GLList.SetItemCount(pList->FlSchGetCount());
                }
                else
                {
                    pList->FlSchCancel();
                    m_GLList.SetItemCount(pList->FlGetCount());
                }
                m_GLList.InvalidateCache();
                m_GLList.ReCalc();
                m_GLList.InvalidateRect();
                CheckVisibleItemMetaData(-1);
            }
            return 0;
            }
    }
    return 0;

    case WM_SIZE:
        m_Lyt.Arrange((int)GetWidthF(), (int)GetHeightF());
        CheckVisibleItemMetaData(-1);
        break;

    case WM_SETFONT:
    {
        m_TBLPlayList.SetTextFormat(GetTextFormat());
        m_EDSearch.SetTextFormat(GetTextFormat());
        m_EDSearchItem.SetTextFormat(GetTextFormat());
    }
    return 0;

    case Dui::EWM_COLORSCHEMECHANGED:
    {
        UpdateDefCover();
        const auto idx = m_TBLPlayList.GetCurrSel();
        if (idx < 0)
            break;
        auto& Lm = App->GetListMgr();
        EckCounter(Lm.GetCount(), i)
        {
            const auto pIl = Lm.At(idx).pImageList.Get();
            if (!pIl)
                continue;
            pIl->ReplaceImage(0, m_pBmpDefCover);
        }
    }
    break;
    case WM_CREATE:
    {
        m_cxIl = (int)Log2PhyF(CxyListCover);
        m_cyIl = m_cxIl;
        UpdateDefCover();
        {
            m_EDSearch.TxSetProp(TXTBIT_MULTILINE, 0, FALSE);
            m_EDSearch.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, 0, CyStdEdit, this);
            m_LytPlayList.Add(&m_EDSearch, { .cyBottomHeight = (int)CxPageIntPadding }, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

            m_TBLPlayList.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, CxListFileList, 0, this, GetWnd());
            m_TBLPlayList.SetItemCount(App->GetListMgr().GetCount());
            m_TBLPlayList.SetBottomExtraSpace(CyPlayPanel);
            m_TBLPlayList.ReCalc();
            m_LytPlayList.Add(&m_TBLPlayList, {}, eck::LF_FILL, 1);
        }
        m_Lyt.Add(&m_LytPlayList, { .cxRightWidth = (int)CxPageIntPadding },
            eck::LF_FIX_WIDTH | eck::LF_FILL_HEIGHT);

        {
            ComPtr<IDWriteTextFormat> pTextFormat;
            App->GetFontFactory().NewFont(pTextFormat.RefOfClear(), eck::Align::Center,
                eck::Align::Center, (float)CyFontNormal, 400);
            pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            m_BTAddFile.Create(L"添加文件", Dui::DES_VISIBLE, 0,
                0, 0, 100, CyStdEdit, this);
            m_BTAddFile.SetTextFormat(pTextFormat.Get());
            m_BTAddFile.SetBitmap(((CWndMain*)GetWnd())->RealizeImage(GImg::Add));
            m_LytTopBar.Add(&m_BTAddFile, {}, eck::LF_FIX);

            m_BTLocate.Create(L"定位当前", Dui::DES_VISIBLE, 0,
                0, 0, 100, CyStdEdit, this);
            m_BTLocate.SetTextFormat(pTextFormat.Get());
            m_BTLocate.SetBitmap(((CWndMain*)GetWnd())->RealizeImage(GImg::Locate));
            m_LytTopBar.Add(&m_BTLocate, { .cxLeftWidth = (int)CxPageIntPadding }, eck::LF_FIX);

            m_LytTopBar.Add(&m_TopBarDummySpace, {}, eck::LF_FILL, 1);

            m_EDSearchItem.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, 200, CyStdEdit, this);
            m_EDSearchItem.SetEventMask(ENM_CHANGE);
            m_LytTopBar.Add(&m_EDSearchItem, {}, eck::LF_FIX | eck::LF_ALIGN_FAR);

            m_LytList.Add(&m_LytTopBar, { .cyBottomHeight = (int)CxPageIntPadding },
                eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

            App->GetFontFactory().NewFont(pTextFormat.RefOfClear(), eck::Align::Near,
                eck::Align::Center, (float)CyFontNormal, 400, TRUE);
            pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            m_GLList.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, 0, 0, this);
            m_GLList.SetView(Dui::CListTemplate::Type::Report);
            constexpr float Width[]{ 270, 150, 150, 50 };
            m_GLList.SetColumnCount(4, Width);
            m_GLList.SetTopExtraSpace(Dui::CListTemplate::CyDefHeader);
            m_GLList.SetBottomExtraSpace(CyPlayPanel);
            m_GLList.SetItemHeight(CyPlayListItem);
            m_GLList.SetSingleSel(FALSE);
            m_GLList.SetTextFormat(pTextFormat.Get());
            m_GLList.GetHeader().SetTextFormat(pTextFormat.Get());
            m_GLList.SetDbgIndex(TRUE);
            m_LytList.Add(&m_GLList, {}, eck::LF_FILL, 1);
        }
        m_Lyt.Add(&m_LytList, { .cxRightWidth = (int)CxPageIntPadding }, eck::LF_FILL, 1);

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
        if (App->GetListMgr().GetCount())
        {
            ReCreateImageList(0, TRUE);
            m_GLList.SetImageList(App->GetListMgr().At(0).pImageList.Get());
            m_TBLPlayList.SelectItemForClick(0);
            CheckVisibleItemMetaData(0);
        }
    }
    break;
    case WM_DPICHANGED:
    {
        m_cxIl = (int)Log2PhyF(CxyListCover);
        m_cyIl = m_cxIl;
        UpdateDefCover();
        ((CWndMain*)GetWnd())->ThreadCtx()->Callback.EnQueueCallback([this]
            {
                ECK_DUILOCK;
                App->GetListMgr().InvalidateImageList();
                const auto idx = m_TBLPlayList.GetCurrSel();
                if (idx < 0)
                    return;
                ReCreateImageList(idx, TRUE);
                const auto& e = App->GetListMgr().At(idx);
                m_GLList.SetImageList(e.pImageList.Get());
                m_GLList.InvalidateRect();
                CheckVisibleItemMetaData(-1);
            });
    }
    break;
    case WM_DESTROY:
        SafeRelease(m_pBmpDefCover);
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}