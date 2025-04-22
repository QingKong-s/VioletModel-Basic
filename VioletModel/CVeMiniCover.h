#pragma once
struct VEN_MINICOVER_CLICK : Dui::DUINMHDR
{

};

class CVeMiniCover : public Dui::CElem
{
private:
	ID2D1Bitmap1
		* m_pBmp{},			// 缓存一个较小尺寸的封面位图
		* m_pBmpCoverUp{};	// 向上箭头
	eck::CEasingCurve* m_pec{};
	BOOLEAN m_bHover{};
	BOOLEAN m_bLBtnDown{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void SetBitmap(ID2D1Bitmap1* pBmp)
	{
		std::swap(m_pBmp, pBmp);
		if (m_pBmp)
			m_pBmp->AddRef();
		if (pBmp)
			pBmp->Release();
	}
};