#pragma once
class CPageList : public CPage
{
private:
	constexpr static int DefCoverIndex{};
	struct LOAD_DATA_ITEM
	{
		eck::CRefStrW rsFile{};
		ULONGLONG Tag{};
		int idxFlat{};
	};
	struct LOAD_DATA_TASK
	{
		eck::CoroTask<void> Task{};
		int idxFlatBegin{};
		int idxFlatEnd{};
	};

	struct LISTFILE
	{
		CPlayList PlayList{};
	};

	Dui::CEdit m_EDSearch{};
	Dui::CTabList m_TBLPlayList{};
	eck::CLinearLayoutV m_LytPlayList{};

	Dui::CButton m_BTAddFile{};
	Dui::CList m_GLList{};
	eck::CLinearLayoutV m_LytList{};

	eck::CLinearLayoutH m_Lyt{};

	std::vector<std::shared_ptr<LISTFILE>> m_vListFile{};

	eck::CRefStrW m_rsDispInfoBuf{};

	int m_cxIl{}, m_cyIl{};
	ID2D1Bitmap1* m_pBmpDefCover{};
	eck::CD2DImageList* m_pIlList{};

	std::vector<LOAD_DATA_TASK> m_vLoadDataTask{};

	/// <summary>
	/// 加载歌曲数据
	/// </summary>
	/// <param name="pListFile">歌曲列表</param>
	/// <param name="pszFile">文件全路径，协程保留其所有权，并在不需要时使用CoTaskMemFree释放</param>
	/// <param name="Tag">任务标记</param>
	eck::CoroTask<void> TskLoadSongData(std::shared_ptr<LISTFILE> pListFile,
		std::vector<LOAD_DATA_ITEM> vItem);
	CPlayList* GetCurrPlayList();
	HRESULT OnMenuAddFile(std::shared_ptr<LISTFILE> pListFile, int idxInsert = -1);
	void OnPlayEvent(const PLAY_EVT_PARAM& e);
	void UpdateDefCover();
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};