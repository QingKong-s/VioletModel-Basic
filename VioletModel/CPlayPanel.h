#pragma once
#include "CVeMiniCover.h"

class CPlayPanel :public Dui::CElem
{
	friend class CWndMain;
private:
	ID2D1SolidColorBrush* m_pBrush{};

	CVeMiniCover m_Cover{};
	Dui::CLabel m_LATitle{};
	Dui::CLabel m_LAArtist{};
	Dui::CLabel m_LATime{};
	Dui::CLabel m_LAWatermark{};

	void OnPlayEvent(const PLAY_EVT_PARAM& e);
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};