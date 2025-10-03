#pragma once
#include "CLyricRendererBase.h"

class CLrGeometryRealization final : public CLyricRendererBase
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
        BOOLEAN bMultiLine{};	// 是否为多行文本，此标志用于尺寸大小改变时的优化
        BOOLEAN bLayoutValid{};	// 文本布局是否有效
        BOOLEAN bGrValid{};		// 几何实现是否有效
    };

    ComPtr<ID2D1DeviceContext1> m_pDC{};
    ComPtr<ID2D1SolidColorBrush> m_pBrush{};
    ComPtr<ID2D1LinearGradientBrush> m_pBrFade{};

    ComPtr<ID2D1GeometryRealization> m_pGrEmptyText{};
    float m_cxEmptyText{};
    float m_cyEmptyText{};

    std::vector<ITEM> m_vItem{};

    void ReCreateFadeBrush();
public:
    HRESULT LrInit(const LRD_INIT& Opt) override;
    void LrBeginDraw() override;
    void LrEndDraw() override;
    void LrItmSetCount(int cItems) override;
    HRESULT LrItmUpdateText(int idx, const Lyric::Line& Line,
        _Out_ LRD_TEXT_METRICS& Met) override;
    void LrItmDraw(const LRD_DRAW& Opt) override;
    HRESULT LrUpdateEmptyText(const LRD_EMTRY_TEXT& Opt) override;
    void LrSetViewSize(float cx, float cy) override;
    void LrDpiChanged(float fNewDpi) override;
    void LrInvalidate() override;
};