#pragma once
class CVeVolumeBar : public Dui::CElem
{
private:
	Dui::CLabel m_LAVol{};
	Dui::CTrackBar m_TrackBar{};

	ID2D1SolidColorBrush* m_pBrush{};
	Dui::CCompositorPageAn* m_pPageAn{};
	eck::CEasingCurve* m_pecShowing{};

	BOOL m_bShow{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void ShowAnimation();

	void OnVolChanged(float fVol);
};