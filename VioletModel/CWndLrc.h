#pragma once
#include "CVioletTheme.h"
#include "CApp.h"
#include "CVeDtLrc.h"


class CWndLrc : public Dui::CDuiWnd, public eck::CFixedTimeLine
{
private:
	Dui::CButton m_BTPrev{}, m_BTPlay{}, m_BTNext{},
		m_BTLock{}, m_BTClose{};
	eck::CLinearLayoutH m_Layout{};
	CVeDtLrc m_Lrc{};
	CVioletTheme* m_pVioletTheme{};
	BOOLEAN m_bInitShow{ TRUE };

	BOOLEAN m_bLock{};
	BOOLEAN m_bShowBk{ TRUE };
	BOOLEAN m_bAnFade{};
	eck::CEasingCurveLite<eck::Easing::FOutCubic> m_AnFade{};

	void OnPlayEvent(const PLAY_EVT_PARAM& e);
public:
	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	LRESULT OnElemEvent(Dui::CElem* pElem, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	LRESULT OnRenderEvent(UINT uMsg, Dui::RENDER_EVENT& e) override;

	void TlTick(int iMs) override;
	BOOL TlIsValid() override { return m_bAnFade; }
};