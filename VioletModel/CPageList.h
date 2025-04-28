#pragma once
class CPageList : public CPage
{
private:
	struct ITEM
	{
		int idxImage{ -1 };
		eck::CRefStrW rsText{};
		ComPtr<IDWriteTextLayout> pTextLayout{};
	};

	struct LIST
	{
		ComPtr<IDWriteTextLayout> pTextLayout{};
		std::vector<ITEM> vItem{};
	};

	Dui::CEdit m_EDSearch{};

	Dui::CTabList m_TBLPlayList{};
	eck::CLinearLayoutV m_LytPlayList{};

	Dui::CList m_GLList{};

	eck::CLinearLayoutH m_Lyt{};
	std::vector<LIST> m_vList{};

	void OnPlayEvent(const PLAY_EVT_PARAM& e);
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};