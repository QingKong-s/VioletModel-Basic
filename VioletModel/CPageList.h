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

	Dui::CTabList m_TBLPlayList{};
	Dui::CGroupList m_GLList{};

	eck::CLinearLayoutH m_Lyt{};
	std::vector<LIST> m_vList{};

	eck::CSrwLock m_LkList{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};