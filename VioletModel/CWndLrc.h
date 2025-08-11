#pragma once
#include "CVioletTheme.h"
#include "CApp.h"
#include "CVeDtLrc.h"


class CWndLrc : public Dui::CDuiWnd
{
private:
	Dui::CButton m_BTPrev{}, m_BTPlay{}, m_BTNext{}, m_BTLock{};
	eck::CLinearLayoutH m_Layout{};
	CVeDtLrc m_Lrc{};
	CVioletTheme* m_pVioletTheme{};
public:
	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	LRESULT OnRenderEvent(UINT uMsg, Dui::RENDER_EVENT& e) override;
};