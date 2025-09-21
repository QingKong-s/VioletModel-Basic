#pragma once
struct LYRIC_TEXT_METRICS
{
	float cxMain{};
	float cyMain{};
	float cxTrans{};
	float cyTrans{};
};
struct LYRIC_DRAW
{
	int idx;
	float x;
	float y;
	float cy;
	float fScale;
	float cxyLineMargin;
	eck::Align eAlignH;

};

struct ILyricRenderer
{
	virtual ~ILyricRenderer() = default;

	virtual HRESULT LrBindDeviceContext(ID2D1DeviceContext* pDC) = 0;
	virtual HRESULT LrBindDeviceContext(ID3D11DeviceContext* pDC) = 0;
	virtual void LrItmSetCount(int cItems) = 0;
	virtual HRESULT LrItmUpdateText(int idx, const Lyric::Line& Line,
		_Out_ LYRIC_TEXT_METRICS& Met) = 0;
	virtual float LrItmDraw(const LYRIC_DRAW& Opt) = 0;
	virtual void LrSetTextColor(D2D1_COLOR_F cr) = 0;
	virtual void LrSetViewSize(float cx, float cy) = 0;
	virtual void LrSetMaxScale(float fScale) = 0;
	virtual void LrDpiChanged(float fNewDpi) = 0;
	virtual void LrInvalidate() = 0;
};