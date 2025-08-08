#pragma once
class CTabPanel : public Dui::CElem
{
private:
	ID2D1SolidColorBrush* m_pBrush{};
	Dui::CLabel m_LAIcon{};
	Dui::CTabList m_TAB{};

	void OnColorSchemeChanged();
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	EckInlineNdCe auto& GetTabList() { return m_TAB; }
};