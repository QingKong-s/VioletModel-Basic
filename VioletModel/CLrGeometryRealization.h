#pragma once
#include "ILyricRenderer.h"

class CLrGeometryRealization final : public ILyricRenderer
{
private:
	struct ITEM
	{
		ComPtr<IDWriteTextLayout> pLayoutMain{};
		ComPtr<IDWriteTextLayout> pLayoutTrans{};
		ComPtr<ID2D1GeometryRealization> pGrMain{};
		ComPtr<ID2D1GeometryRealization> pGrTrans{};
		float cxMain{};
		float cxTrans{};
		BOOLEAN bMultiLine{};// 是否为多行文本，此标志用于尺寸大小改变时的优化
		BOOLEAN bLayoutValid{};// 文本布局是否有效
		BOOLEAN bGrValid{};// 几何实现是否有效
	};

	ComPtr<ID2D1DeviceContext1> m_pDC{};
	ComPtr<ID2D1SolidColorBrush> m_pBrush{};
	ComPtr<IDWriteTextFormat> m_pTextFormatMain{};
	ComPtr<IDWriteTextFormat> m_pTextFormatTrans{};
	float m_cxView{};
	float m_cyView{};
	float m_fMaxScale{ 1.f };

	std::vector<ITEM> m_vItem{};
public:
	HRESULT LrBindDeviceContext(ID2D1DeviceContext* pDC) override;
	HRESULT LrBindDeviceContext(ID3D11DeviceContext* pDC) override
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}
	void LrItmSetCount(int cItems) override;
	HRESULT LrItmUpdateText(int idx, const Lyric::Line& Line,
		_Out_ LYRIC_TEXT_METRICS& Met) override;
	void LrItmDraw(const LYRIC_DRAW& Opt) override;
	void LrSetTextColor(D2D1_COLOR_F cr) override;
	void LrSetViewSize(float cx, float cy) override;
	void LrSetMaxScale(float fScale) override;
	void LrDpiChanged(float fNewDpi) override;
	void LrInvalidate() override;
};