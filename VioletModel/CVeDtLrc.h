#pragma once
#include "CLyric.h"

class CVeDtLrc : public Dui::CElem
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

	LINE m_TextCache[2]{};
	STATIC_LINE m_StaticLine{};

	int m_idxCurr{ -1 };
	BOOLEAN m_bStaticLine{};		// 绘制静态行，当无歌词时设置为TRUE
	BOOLEAN m_bAutoWrap{};			// 自动换行
	BOOLEAN m_bShowTrans{ TRUE };	// 显示翻译

	void InvalidateCache();

	float DrawLrcLine(int idxLrc, float y, BOOL bSecondLine);

	void DrawStaticLine(float y);
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void LrcSetCurrentLine(int idx);

	void SetTextFormatTranslation(IDWriteTextFormat* pTf);
	EckInlineNdCe auto GetTextFormatTranslation() const { return m_pTfTranslation; }

	void SetLyric(CLyric* pLrc);
	EckInlineNdCe auto GetLyric() const { return m_pLrc; }
};