#include "pch.h"
#include "CLrGeometryRealization.h"

HRESULT CLrGeometryRealization::LrBindDeviceContext(ID2D1DeviceContext* pDC)
{
	if (SUCCEEDED(pDC->QueryInterface(m_pDC.AddrOfClear())))
		return S_OK;
	pDC->CreateSolidColorBrush({}, m_pBrush.AddrOfClear());
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

void CLrGeometryRealization::LrItmDraw(const LYRIC_DRAW& Opt)
{
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