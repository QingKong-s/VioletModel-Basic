#pragma once
class CVeLrc : public Dui::CElem
{
public:
	enum : size_t
	{
		CriNormal,
		CriHiLight,
		CriMax
	};
private:
	struct ITEM
	{
		// 不要修改前两个字段的位置
		ComPtr<IDWriteTextLayout> pLayout{};
		ComPtr<IDWriteTextLayout> pLayoutTrans{};
		ComPtr<ID2D1GeometryRealization> pGr{};
		float x{};
		float y{};
		float cy{};
		float cx{};

		float cxTrans{};
		BITBOOL bSel : 1{};
		BITBOOL bCacheValid : 1{};

	};

	ID2D1DeviceContext1* m_pDC1{};
	ID2D1SolidColorBrush* m_pBrush{};
	ID2D1GeometryRealization* m_pGrEmptyText{};

	IDWriteTextFormat* m_pTextFormatTrans{};

	Dui::CScrollBar m_SB{};
	eck::CInertialScrollView* m_psv{};

	std::shared_ptr<std::vector<eck::LRCINFO>> m_pvLrc{};
	std::vector<ITEM> m_vItem{};
	int m_idxTop{ -1 };
	int m_idxHot{ -1 };
	int m_idxMark{ -1 };
	int m_idxPrevCurr{ -1 };

	eck::CEasingAn<eck::Easing::FOutCubic> m_AnEnlarge{};
	int m_idxPrevAnItem{ -1 },
		m_idxCurrAnItem{ -1 };
	float m_fAnValue{ 1.f };
	BOOL m_bEnlarging{};

	int m_tMouseIdle{};

	float m_fPlayingItemScale{ 1.2f };
	eck::Align m_eAlignH{ eck::Align::Near };

	D2D1_COLOR_F m_Color[CriMax]{};


	static void ScrollProc(int iPos, int iPrevPos, LPARAM lParam);

	void FillItemBkg(int idx, const D2D1_RECT_F& rc);

	void CalcTopItem();

	BOOL DrawItem(int idx, float& y);

	void BeginMouseIdleDetect();

	int HitTest(POINT pt);

	void GetItemRect(int idx, RECT& rc);

	float GetItemY(int idx);

	void InvalidateItem(int idx);

	void ScrollToCurrPos();

	void LayoutItems();

	void Scrolled();
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// 调用方检查歌词内容，若为空则调用此函数设置空提示文本
	HRESULT UpdateEmptyText(std::wstring_view svEmptyText);

	// 调用方启动定时器检查当前时间对应的歌词行，每次检查后将结果传递到此方法
	HRESULT LrcTick(int idxCurr);

	HRESULT LrcInit(std::shared_ptr<std::vector<eck::LRCINFO>> pvLrc);

	void LrcClear();

	void SetTextFormatTrans(IDWriteTextFormat* pTf);

	EckInlineCe void SetColor(size_t idx, const D2D1_COLOR_F& cr)
	{
		if (idx >= CriMax)
			return;
		m_Color[idx] = cr;
	}

	EckInlineCe void SetColor(_In_reads_(CriMax) const D2D1_COLOR_F* pcr)
	{
		for (size_t i = 0; i < CriMax; ++i)
			m_Color[i] = pcr[i];
	}
};