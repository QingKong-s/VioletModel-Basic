#pragma once
#include "CMiniCover.h"

class CPlayPanel :public Dui::CElem
{
private:
	ID2D1SolidColorBrush* m_pBrush{};

	CMiniCover m_Cover{};
	Dui::CLabel m_LATitle{};
	Dui::CLabel m_LAArtist{};
	Dui::CLabel m_LATime{};
	Dui::CLabel m_LAWatermark{};

	Dui::CCircleButton
		m_BTPrev{},
		m_BTPlay{},
		m_BTNext{},
		m_BTLrc{},
		m_BTVol{};

	void UpdateButtonImageSize();
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};