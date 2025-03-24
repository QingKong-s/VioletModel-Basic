#pragma once
#include "CMiniCover.h"

class CPlayPanel :public Dui::CElem
{
private:
	ID2D1SolidColorBrush* m_pBrush{};

	CMiniCover m_Cover{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};