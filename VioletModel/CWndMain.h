#pragma once
#include "CPage.h"
#include "CPageMain.h"
#include "CPageList.h"

class CWndMain : public Dui::CDuiWnd
{
public:
	enum class Page
	{
		Main,
		List,
		Max
	};
private:
	Dui::CTitleBar m_TitleBar{};
	Dui::CLabel m_LAPageTitle{};
	CTabPanel m_TabPanel{};

	CPageMain m_PageMain{};
	CPlayPanel m_PlayPanel{};
	CPageList m_PageList{};

	Dui::CTrackBar m_TBProgress{};

	ID2D1Bitmap* m_vBmpRealization[(size_t)GImg::Max]{};
	IDWriteTextFormat* m_pTfCenter{};
	IDWriteTextFormat* m_pTfLeft{};

	CPage* m_vPage[(size_t)Page::Max]{ &m_PageMain, &m_PageList };

	CPage* m_pAnPage{};
	eck::CEasingCurve* m_pecPage{};
private:
	void ClearRes();

	BOOL OnCreate(HWND hWnd, CREATESTRUCT* pcs);

	void ShowPage(Page ePage, BOOL bAnimate);

	void ClearPageAnimation();
public:
	~CWndMain();

	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	BOOL PreTranslateMessage(const MSG& Msg) override;

	LRESULT OnElemEvent(Dui::CElem* pElem, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	ID2D1Bitmap* RealizeImg(GImg n)
	{
		if (!m_vBmpRealization[(size_t)n])
		{
			GetD2D().GetDC()->CreateBitmapFromWicBitmap(App->GetImg(n),
				&m_vBmpRealization[(size_t)n]);
		}
		return m_vBmpRealization[(size_t)n];
	}

	EckInline constexpr auto TfGetCenter() const noexcept { return m_pTfCenter; }
	EckInline constexpr auto TfGetLeft() const noexcept { return m_pTfLeft; }
};