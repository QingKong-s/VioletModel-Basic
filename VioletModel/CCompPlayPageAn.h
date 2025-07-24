#pragma once
class CCompPlayPageAn final : public Dui::CCompositorCornerMapping
{
private:
	ComPtr<ID2D1Bitmap1> pBitmapOverlay{};
public:
	void PostRender(Dui::COMP_RENDER_INFO& cri) override
	{
		cri.pDC->DrawBitmap(cri.pBitmap, cri.rcDst, 1.f,
			D2D1_INTERPOLATION_MODE_LINEAR, cri.rcSrc, (D2D1_MATRIX_4X4_F*)&Mat);
		if (pBitmapOverlay.Get())
			cri.pDC->DrawBitmap(pBitmapOverlay.Get(), cri.rcDst, 1.f - Opacity,
				D2D1_INTERPOLATION_MODE_LINEAR, nullptr, (D2D1_MATRIX_4X4_F*)&Mat);
	}

	void SetOverlayBitmap(ID2D1Bitmap1* pBmp)
	{
		pBitmapOverlay = pBmp;
	}
};