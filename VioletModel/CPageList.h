#pragma once
class CPageList : public CPage
{
private:
	constexpr static int DefCoverIndex{};
	struct LIST_INFO
	{
		ComPtr<eck::CD2DImageList> pIl;
	};

	Dui::CEdit m_EDSearch{};
	Dui::CTabList m_TBLPlayList{};
	eck::CLinearLayoutV m_LytPlayList{};

	Dui::CButton m_BTAddFile{};
	Dui::CList m_GLList{};
	eck::CLinearLayoutV m_LytList{};

	eck::CLinearLayoutH m_Lyt{};

	eck::CRefStrW m_rsDispInfoBuf{};

	int m_cxIl{}, m_cyIl{};
	ID2D1Bitmap1* m_pBmpDefCover{};
	std::vector<LIST_INFO> m_vListInfo{};

	/// <summary>
	/// 加载歌曲数据
	/// </summary>
	/// <param name="pList">列表</param>
	/// <param name="vItem">待加载的歌曲索引，协程移动此参数使自己拥有所有权</param>
	eck::CoroTask<void> TskLoadSongData(std::shared_ptr<CPlayList> pList,
		ComPtr<eck::CD2DImageList> pIl, std::vector<int>&& vItem);

	CPlayList* GetCurrPlayList();
	std::shared_ptr<CPlayList> GetCurrPlayListShared();

	HRESULT OnMenuAddFile(CPlayList* pList, int idxInsert = -1);

	void OnPlayEvent(const PLAY_EVT_PARAM& e);

	void UpdateDefCover();

	void ReCreateImageList(int idx, BOOL bForce);

	void LoadMetaData(int idxBegin, int idxEnd, int idxList = -1);

	void CheckVisibleItemMetaData(int idxList);
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};