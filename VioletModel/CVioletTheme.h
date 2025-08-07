#pragma once
class CVioletTheme final : public eck::CUnknown<CVioletTheme, Dui::ITheme>
{
private:
	ComPtr<Dui::ITheme> m_pDefTheme{};
	ComPtr<ID2D1DeviceContext> m_pDC{};
	ComPtr<ID2D1SolidColorBrush> m_pBrush{};
public:
	void Init(ID2D1DeviceContext* pDC, Dui::ITheme* pDefTheme);
	void SetDefaultTheme(Dui::ITheme* pDefTheme);

	HRESULT DrawBackground(Dui::Part ePart, Dui::State eState,
		const D2D1_RECT_F& rc, _In_opt_ const Dui::DTB_OPT* pOpt) override;
	HRESULT SetColorizationColor(const D2D1_COLOR_F& cr) override;
	HRESULT GetColorizationColor(_Out_ D2D1_COLOR_F& cr) override;
	HRESULT GetColor(Dui::Part ePart, Dui::State eState,
		eck::ClrPart eClrPart, _Out_ D2D1_COLOR_F& cr) override;
	HRESULT GetSysColor(Dui::SysColor eSysColor, _Out_ D2D1_COLOR_F& cr) override;
	float GetMetrics(Dui::Metrics eMetrics) override;
};