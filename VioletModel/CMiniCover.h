#pragma once
class CMiniCover : public Dui::CElem
{
private:
	ID2D1Bitmap1* m_pBmp{};// 缓存一个较小尺寸的封面位图
	eck::CEasingCurve* m_pec{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};