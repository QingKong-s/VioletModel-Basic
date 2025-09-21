#include "pch.h"
#include "CLrGeometryRealization.h"

HRESULT CLrGeometryRealization::LrBindDeviceContext(ID2D1DeviceContext* pDC)
{
	if (SUCCEEDED(pDC->QueryInterface(m_pDC.AddrOfClear())))
	{
		pDC->CreateSolidColorBrush({}, m_pBrush.AddrOfClear());
		return S_OK;
	}
	return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

void CLrGeometryRealization::LrItmSetCount(int cItems)
{
	m_vItem.clear();
	m_vItem.resize(cItems);
}

HRESULT CLrGeometryRealization::LrItmUpdateText(int idx,
	const Lyric::Line& Line, _Out_ LYRIC_TEXT_METRICS& Met)
{
	EckAssert(idx >= 0 && idx < (int)m_vItem.size());
	DWRITE_TEXT_METRICS Metrics{};
	auto& e = m_vItem[idx];
	e.bLayoutValid = TRUE;
	e.bGrValid = FALSE;
	eck::g_pDwFactory->CreateTextLayout(Line.pszLrc, Line.cchLrc,
		m_pTextFormatMain.Get(), m_cxView, m_cyView, e.pLayoutMain.AddrOfClear());
	e.pLayoutMain->GetMetrics(&Metrics);
	Met.cxMain = e.cxMain = Metrics.width;
	Met.cyMain = Metrics.height;

	if (Line.pszTranslation && Line.cchTranslation)
	{
		eck::g_pDwFactory->CreateTextLayout(Line.pszTranslation,
			Line.cchTranslation, m_pTextFormatTrans.Get(),
			m_cxView, m_cyView, e.pLayoutTrans.AddrOfClear());
		e.pLayoutTrans->GetMetrics(&Metrics);
		Met.cxTrans = e.cxTrans = Metrics.width;
		Met.cyTrans = Metrics.height;
	}
	else
		e.pLayoutTrans.Clear();
	return S_OK;
}

float CLrGeometryRealization::LrItmDraw(const LYRIC_DRAW& Opt)
{
	/*auto& e = m_vItem[Opt.idx];
	D2D1_MATRIX_3X2_F Mat0;
	m_pDC->GetTransform(&Mat0);

	D2D1_POINT_2F ptScale{};
	ptScale.y = Opt.cy / 2.f - Opt.cxyLineMargin;
	switch (Opt.eAlignH)
	{
	case eck::Align::Center:
		ptScale.x = m_cxView / 2.f;
		break;
	case eck::Align::Far:
		ptScale.x = m_cxView;
		break;
	}

	D2D1_MATRIX_3X2_F Mat{ Mat0 };
	const auto cyExtra = (Opt.cy - Opt.cxyLineMargin * 2.f) *
		(Opt.fScale - 1.f) / 2.f;
	Mat.dx += Opt.cxyLineMargin;
	Mat.dy += (Opt.y + Opt.cxyLineMargin + cyExtra);

	if (!e.bGrValid)
	{
		constexpr float cyPadding[]{ 5.f,0.f };

		float x[2]{};
		switch (Opt.eAlignH)
		{
		case eck::Align::Center:
			x[0] = (m_cxView - m_cxView / Opt.fScale) / 2.f;
			x[1] = (m_cxView - e.cxTrans) / 2.f;
			break;
		case eck::Align::Far:
			x[0] = (m_cxView - m_cxView / Opt.fScale);
			x[1] = (m_cxView - e.cxTrans);
			break;
		}

		ComPtr<ID2D1PathGeometry> pPathGeometry;
		eck::GetTextLayoutPathGeometry(0, 2, cyPadding,
			m_pDC, x, 0.f, pPathGeometry.RefOf());
		float xDpi, yDpi;
		m_pDC->GetDpi(&xDpi, &yDpi);
		m_pDC1->CreateFilledGeometryRealization(
			pPathGeometry.Get(),
			D2D1::ComputeFlatteningTolerance(
				D2D1::Matrix3x2F::Identity(), xDpi, yDpi, m_fPlayingItemScale),
			Item.pGr.AddrOfClear());
		Item.bCacheValid = TRUE;
	}

	Dui::State eState;
	if (m_vItem[idx].bSel)
		if (idx == m_idxHot)
			eState = Dui::State::HotSelected;
		else
			eState = Dui::State::Selected;
	else
		if (idx == m_idxHot || Item.bAnSelBkg)
			eState = Dui::State::Hot;
		else
			eState = Dui::State::None;
	if (eState != Dui::State::None)
	{
		D2D1_RECT_F rc;
		ItmGetRect(idx, rc);
		Dui::DTB_OPT Opt;
		if (Item.bAnSelBkg && eState == Dui::State::Hot)
		{
			const auto dxy = -m_cxyLineMargin * Item.kAnSelBkg;
			eck::InflateRect(rc, dxy, dxy);
			Opt.uFlags = Dui::DTBO_NEW_OPACITY;
			Opt.fOpacity = 1.f - Item.kAnSelBkg;
		}
		else
			Opt.uFlags = Dui::DTBO_NONE;
		GetTheme()->DrawBackground(Dui::Part::ListItem, eState, rc, &Opt);
	}
	if (idx == m_idxPrevAnItem)
	{
		const auto k = m_fPlayingItemScale + 1.f - m_fAnValue;
		m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
		m_pBrush->SetColor(InterpolateColor(
			m_Color[CriNormal], m_Color[CriHiLight], 1.f - m_fAnValue));

	}
	else if (idx == m_idxCurrAnItem)
	{
		const auto k = m_fAnValue;
		m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
		m_pBrush->SetColor(InterpolateColor(
			m_Color[CriNormal], m_Color[CriHiLight], m_fAnValue));
	}
	else
	{
		if (m_bScrollExpand || MiIsManualScroll())
		{
			m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(
				m_kScrollExpand, m_kScrollExpand, ptScale) * Mat);
		}
		else
			m_pDC->SetTransform(Mat);
		m_pBrush->SetColor(m_Color[m_idxCurr == idx ? CriHiLight : CriNormal]);
	}
	m_pDC1->DrawGeometryRealization(Item.pGr.Get(), m_pBrush);
	m_pDC->SetTransform(Mat0);
#ifdef _DEBUG
	WCHAR szDbg[eck::CchI32ToStrBufNoRadix2];
	const auto cchDbg = swprintf_s(szDbg, L"%d", idx);
	D2D1_RECT_F rcDbg;
	ItmGetRect(idx, rcDbg);
	m_pBrush->SetColor(D2D1::ColorF{ (ItmIsDelaying() && ItmInDelayRange(idx)) ?
		D2D1::ColorF::Green : D2D1::ColorF::Red });
	m_pDC->DrawTextW(szDbg, cchDbg, GetTextFormat(), rcDbg, m_pBrush);
#endif
	return Item.y + Item.cy + m_cyLinePadding;*/
return 0.f;
}

void CLrGeometryRealization::LrSetTextColor(D2D1_COLOR_F cr)
{
	m_pBrush->SetColor(cr);
}

void CLrGeometryRealization::LrSetViewSize(float cx, float cy)
{
	m_cyView = cy;
	if (eck::FloatEqual(m_cxView, cx))
		return;
	if (cx < m_cxView)
		for (auto& e : m_vItem)
		{
			if (e.bMultiLine || std::max(e.cxMain, e.cxTrans) > cx)
			{
				e.bGrValid = FALSE;
				e.bLayoutValid = FALSE;
			}
		}
	else
		for (auto& e : m_vItem)
		{
			if (e.bMultiLine)
			{
				e.bGrValid = FALSE;
				e.bLayoutValid = FALSE;
			}
		}
	m_cxView = cx;
}

void CLrGeometryRealization::LrSetMaxScale(float fScale)
{
	m_fMaxScale = fScale;
	LrInvalidate();
}

void CLrGeometryRealization::LrDpiChanged(float fNewDpi)
{
	LrInvalidate();
}

void CLrGeometryRealization::LrInvalidate()
{
	for (auto& e : m_vItem)
		e.bGrValid = FALSE;
}