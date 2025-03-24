#pragma once
class CTabPanel : public Dui::CElem
{
private:
	ID2D1SolidColorBrush* m_pBrush{};
	Dui::CLabel m_LAIcon{};
	Dui::CTabList m_TAB{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};