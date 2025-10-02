#pragma once
class CPageList : public CPage
{
private:
	constexpr static int DefCoverIndex{};
	struct LIST_INFO
	{
		ComPtr<eck::CD2DImageList> pIl;
	};

	struct TSKPARAM_LOAD_META_DATA
	{
		std::shared_ptr<CPlayList> pList;
		int idxBeginDisplay{};
		int idxEndDisplay{};
		ComPtr<eck::CD2DImageList> pIl;
		std::vector<int> vItem;
    };;

	Dui::CEdit m_EDSearch{};
	Dui::CTabList m_TBLPlayList{};
	eck::CLinearLayoutV m_LytPlayList{};

	Dui::CButton m_BTAddFile{};
	eck::CLayoutDummy m_TopBarDummySpace{};
	Dui::CEdit m_EDSearchItem{};
	eck::CLinearLayoutH m_LytTopBar{};
	Dui::CList m_GLList{};
	eck::CLinearLayoutV m_LytList{};

	eck::CLinearLayoutH m_Lyt{};

	eck::CRefStrW m_rsDispInfoBuf{};

	int m_cxIl{}, m_cyIl{};
	ID2D1Bitmap1* m_pBmpDefCover{};
	std::vector<LIST_INFO> m_vListInfo{};

    eck::CoroTask<void> TskLoadSongData(TSKPARAM_LOAD_META_DATA&& Param);

	CPlayList* GetCurrPlayList();
	std::shared_ptr<CPlayList> GetCurrPlayListShared();

	HRESULT OnMenuAddFile(CPlayList* pList, int idxInsert = -1);

	void UpdateDefCover();

	void ReCreateImageList(int idx, BOOL bForce);

	void LoadMetaData(int idxBegin, int idxEnd, int idxList = -1);

	void CheckVisibleItemMetaData(int idxList);
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};