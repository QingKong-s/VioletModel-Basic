#pragma once
#include "CLyric.h"

struct NM_DTL_GET_TIME : Dui::DUINMHDR
{
	float fTime;
};

class CVeDtLrc : public Dui::CElem, public eck::CFixedTimeLine
{
public:
	constexpr static int c_InvalidCacheIdx = std::numeric_limits<int>::min();
private:
	enum : size_t
	{
		BriMain,
		BriTrans,
		BriMainHiLight,
		BriTransHiLight,
		BriBorder,
		BriShadow,

		BriMax
	};
	struct LINE
	{
		IDWriteTextLayout* pLayout;
		IDWriteTextLayout* pLayoutTrans;
		ID2D1GeometryRealization* pGrS;
		ID2D1GeometryRealization* pGrSTrans;
		ID2D1GeometryRealization* pGrF;
		ID2D1GeometryRealization* pGrFTrans;
		D2D1_SIZE_F size;
		D2D1_SIZE_F sizeTrans;
		int idxLrc;
		BOOLEAN bTooLong;
		BOOLEAN bTooLongTrans;
		BOOLEAN bScrolling;
	};
	struct STATIC_LINE
	{
		eck::CRefStrW rsText;
		ID2D1GeometryRealization* pGrF;
		ID2D1GeometryRealization* pGrS;
		float cx;
		BOOL bValid;
	};

	CLyric* m_pLrc{};
	ID2D1DeviceContext1* m_pDC1{};
	IDWriteTextFormat* m_pTfTranslation{};

	ID2D1Brush* m_pBrush[BriMax]{};

	LINE m_Line[2]{};
	STATIC_LINE m_StaticLine{};

	float m_cxOutline{ 2.f };
	float m_cyLinePadding{ 4.f };
	float m_dxyShadow{ 3.f };

	int m_idxCurr{ -1 };
	BOOLEAN m_bStaticLine{};		// 绘制静态行，当无歌词时设置为TRUE
	BOOLEAN m_bTooLong{};			// 歌词太长，需要滚动显示

	BOOLEAN m_bAutoWrap{};			// 自动换行
	BOOLEAN m_bShowTrans{ TRUE };	// 显示翻译
	BOOLEAN m_bShadow{ TRUE };		// 显示阴影
	eck::Align m_eAlign[2]{ eck::Align::Near,eck::Align::Far };


	void InvalidateCache();

	float DrawLrcLine(int idxLrc, float y, BOOL bSecondLine);

	void DrawStaticLine(float y);

	void DrawTextGeometry(ID2D1GeometryRealization* pGrS, ID2D1GeometryRealization* pGrF,
		float dx, float dy, ID2D1Brush* pBrFill);
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void Tick(int iMs) override;

	BOOL IsValid() override { return m_bTooLong; }

	HRESULT LrcSetCurrentLine(int idx);
	void LrcSetEmptyText(std::wstring_view svEmptyText);

	void SetTextFormatTrans(IDWriteTextFormat* pTf);
	EckInlineNdCe auto GetTextFormatTrans() const { return m_pTfTranslation; }

	void SetLyric(CLyric* pLrc);
	EckInlineNdCe auto GetLyric() const { return m_pLrc; }
};