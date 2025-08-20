#include "pch.h"
#include "CVioletTheme.h"
#include "CApp.h"

void CVioletTheme::Init(ID2D1DeviceContext* pDC, Dui::ITheme* pDefTheme)
{
	m_pDC = pDC;
	m_pDefTheme = pDefTheme;
	m_pDC->CreateSolidColorBrush({}, &m_pBrush);
}

void CVioletTheme::SetDefaultTheme(Dui::ITheme* pDefTheme)
{
	m_pDefTheme = pDefTheme;
}

HRESULT CVioletTheme::DrawBackground(Dui::Part ePart, Dui::State eState,
	const D2D1_RECT_F& rc, _In_opt_ const Dui::DTB_OPT* pOpt)
{
	D2D1_ELLIPSE Ell;
	switch (ePart)
	{
	case Dui::Part::CircleButton:
	{
		Ell.point = D2D1::Point2F(rc.left + rc.right / 2, rc.top + rc.bottom / 2);
		Ell.radiusX = rc.right / 2;
		Ell.radiusY = rc.bottom / 2;
		switch (eState)
		{
		case Dui::State::Normal:
			m_pBrush->SetColor(App->GetColor(GPal::PlayBtnBkNormal));
			break;
		case Dui::State::Hot:
			m_pBrush->SetColor(App->GetColor(GPal::PlayBtnBkHot));
			break;
		case Dui::State::Selected:
			m_pBrush->SetColor(App->GetColor(GPal::PlayBtnBkSelected));
			break;
		default:
			goto DoDef;
		}
		m_pDC->FillEllipse(Ell, m_pBrush.Get());
	}
	return S_OK;

	case Dui::Part::Button:
	{
		if (eState == Dui::State::Normal)
			return S_OK;
	}
	break;

	case Dui::Part::ScrollThumb:
	{
		if (eState == Dui::State::Normal)// 颜色调明显一点，不然看不清
		{
			m_pBrush->SetColor(App->GetColor(GPal::ScrollBarThumb));
			m_pDC->FillRectangle(rc, m_pBrush.Get());
			return S_OK;
		}
	}
	break;
	}
DoDef:
	return m_pDefTheme->DrawBackground(ePart, eState, rc, pOpt);
}

HRESULT CVioletTheme::SetColorizationColor(const D2D1_COLOR_F& cr)
{
	return m_pDefTheme->SetColorizationColor(cr);
}

HRESULT CVioletTheme::GetColorizationColor(_Out_ D2D1_COLOR_F& cr)
{
	return m_pDefTheme->GetColorizationColor(cr);
}

HRESULT CVioletTheme::GetColor(Dui::Part ePart, Dui::State eState,
	eck::ClrPart eClrPart, _Out_ D2D1_COLOR_F& cr)
{
	return m_pDefTheme->GetColor(ePart, eState, eClrPart, cr);
}

HRESULT CVioletTheme::GetSysColor(Dui::SysColor eSysColor, _Out_ D2D1_COLOR_F& cr)
{
	return m_pDefTheme->GetSysColor(eSysColor, cr);
}

float CVioletTheme::GetMetrics(Dui::Metrics eMetrics)
{
	return m_pDefTheme->GetMetrics(eMetrics);
}