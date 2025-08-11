#include "pch.h"
#include "CVeDtLrc.h"


void CVeDtLrc::InvalidateCache()
{
	for (auto& e : m_TextCache)
		e.idxLrc = c_InvalidCacheIdx;
}

float CVeDtLrc::DrawLrcLine(int idxLrc, float y, BOOL bSecondLine)
{
	EckAssert(bSecondLine == 1 || bSecondLine == 0);
	const auto pLyric = GetLyric();
	auto& Cache = m_TextCache[bSecondLine];
	const BOOL bHiLight = (idxLrc == m_idxCurr);

	const float cxMax = GetWidthF();
	const float cyMax = GetHeightF();

	if (Cache.idxLrc != idxLrc)// 更新缓存
	{
		LYRIC_LINE Lrc;
		pLyric->LrcGetLyric(idxLrc, Lrc);
		float xDpi, yDpi;
		m_pDC1->GetDpi(&xDpi, &yDpi);

		const float fTolerance = D2D1::ComputeFlatteningTolerance(
			D2D1::Matrix3x2F::Identity(), xDpi, yDpi, 1.f);

		const float cxStroke = 2.f;

		Cache.idxLrc = idxLrc;
		SafeRelease(Cache.pLayout);
		SafeRelease(Cache.pLayoutTrans);

		if (!Lrc.pszLrc)
		{
			constexpr WCHAR EmptyText[]{ L"♪♪♪" };
			Lrc.pszLrc = EmptyText;
			Lrc.cchLrc = ARRAYSIZE(EmptyText) - 1;
		}

		//-----------重建文本布局
		DWRITE_TEXT_METRICS tm;
		eck::g_pDwFactory->CreateTextLayout(Lrc.pszLrc, Lrc.cchLrc,
			GetTextFormat(), cxMax, cyMax, &Cache.pLayout);

		Cache.pLayout->GetMetrics(&tm);
		Cache.bTooLong = (tm.width > cxMax);
		Cache.size = { tm.width,tm.height };

		//-----------重建几何实现
		ComPtr<ID2D1PathGeometry1> pPath;
		eck::GetTextLayoutPathGeometry(Cache.pLayout, m_pDC1, 0, 0, pPath.RefOfClear());
		SafeRelease(Cache.pGrF);
		SafeRelease(Cache.pGrS);
		m_pDC1->CreateFilledGeometryRealization(pPath.Get(), fTolerance, &Cache.pGrF);
		m_pDC1->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
			cxStroke, nullptr, &Cache.pGrS);

		if (!Lrc.pszTranslation)
		{
			//-----------重建文本布局
			eck::g_pDwFactory->CreateTextLayout(Lrc.pszLrc, Lrc.cchLrc,
				GetTextFormatTranslation(), cxMax, cyMax, &Cache.pLayoutTrans);
			Cache.bTooLongTrans = (tm.width > cxMax);
			Cache.sizeTrans = { tm.width,tm.height };

			//-----------重建几何实现
			eck::GetTextLayoutPathGeometry(Cache.pLayoutTrans, m_pDC1, 0, 0, pPath.RefOfClear());

			SafeRelease(Cache.pGrFTrans);
			SafeRelease(Cache.pGrSTrans);
			m_pDC1->CreateFilledGeometryRealization(pPath.Get(), fTolerance, &Cache.pGrFTrans);
			m_pDC1->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
				cxStroke, nullptr, &Cache.pGrSTrans);
		}
	}

	LYRIC_CURR_TIME CurrTime;
	pLyric->LrcGetCurrentTimeInfo(idxLrc, CurrTime);

	const float cxMaxHalf = cxMax / 2.f;
	float dx;
	if (Cache.bTooLong)
	{
		dx = (CurrTime.fCurrTime - CurrTime.fLrcTime) *
			Cache.size.width / CurrTime.fLrcDuration;
		if (dx < cxMaxHalf)
			dx = 0.f;
		else if (dx > Cache.size.width - cxMaxHalf)
			dx = cxMax - Cache.size.width;
		else
			dx = cxMaxHalf - dx;
	}
	else
		dx = 0.f;

	D2D1_MATRIX_3X2_F Mat;
	m_pDC1->GetTransform(&Mat);
	Mat.dx += dx;
	Mat.dy += y;
	m_pDC1->SetTransform(Mat);
	m_pDC1->DrawGeometryRealization(Cache.pGrS, m_pBrush[BriBorder]);
	m_pDC1->DrawGeometryRealization(Cache.pGrF,
		m_pBrush[bHiLight ? BriMainHiLight : BriMain]);
	m_pDC1->SetTransform(Mat);

	float cy = Cache.size.height;
	if (Cache.pLayoutTrans)
	{
		const float yNew = y + cy;
		if (Cache.bTooLongTrans)
		{
			dx = (CurrTime.fCurrTime - CurrTime.fLrcTime) *
				Cache.sizeTrans.width / CurrTime.fLrcDuration;
			if (dx < cxMaxHalf)
				dx = 0;
			else if (dx > Cache.sizeTrans.width - cxMaxHalf)
				dx = cxMax - Cache.sizeTrans.width;
			else
				dx = cxMaxHalf - dx;
		}
		else
			dx = 0.f;

		m_pDC1->GetTransform(&Mat);
		Mat.dx += dx;
		Mat.dy += yNew;
		m_pDC1->DrawGeometryRealization(Cache.pGrSTrans, m_pBrush[BriBorder]);
		m_pDC1->DrawGeometryRealization(Cache.pGrFTrans,
			m_pBrush[bHiLight ? BriTransHiLight : BriTrans]);
		m_pDC1->SetTransform(Mat);

		cy += Cache.sizeTrans.height;
	}
#pragma warning(pop)
	return cy;
}

void CVeDtLrc::DrawStaticLine(float y)
{
	auto& Cache = m_StaticLine;
	if (!Cache.bValid)
	{
		Cache.bValid = TRUE;
		float xDpi, yDpi;
		m_pDC1->GetDpi(&xDpi, &yDpi);

		const float fTolerance = D2D1::ComputeFlatteningTolerance(
			D2D1::Matrix3x2F::Identity(), xDpi, yDpi, 1.f);

		const float cxStroke = 2.f;

		ComPtr<IDWriteTextLayout> pLayout;
		eck::g_pDwFactory->CreateTextLayout(Cache.rsText.Data(),
			Cache.rsText.Size(), GetTextFormat(),
			GetWidthF(), GetHeightF(), &pLayout);
		DWRITE_TEXT_METRICS tm;
		pLayout->GetMetrics(&tm);
		Cache.cx = tm.width;
		//-----------重建几何实现
		ComPtr<ID2D1PathGeometry1> pPath;
		eck::GetTextLayoutPathGeometry(pLayout.Get(), m_pDC1, 0, 0, pPath.RefOf());
		SafeRelease(Cache.pGrF);
		SafeRelease(Cache.pGrS);
		m_pDC1->CreateFilledGeometryRealization(pPath.Get(), fTolerance, &Cache.pGrF);
		m_pDC1->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
			cxStroke, nullptr, &Cache.pGrS);
	}

	D2D1_MATRIX_3X2_F Mat;
	m_pDC1->GetTransform(&Mat);
	Mat.dx += 0;
	Mat.dy += y;
	m_pDC1->SetTransform(Mat);
	m_pDC1->DrawGeometryRealization(Cache.pGrS, m_pBrush[BriBorder]);
	m_pDC1->DrawGeometryRealization(Cache.pGrF, m_pBrush[BriMain]);
	m_pDC1->SetTransform(Mat);
}

LRESULT CVeDtLrc::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		m_pDC->QueryInterface(&m_pDC1);
		InvalidateCache();

	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeDtLrc::LrcSetCurrentLine(int idx)
{
	m_idxCurr = idx;
}

void CVeDtLrc::SetTextFormatTranslation(IDWriteTextFormat* pTf)
{
	ECK_DUILOCK;
	std::swap(m_pTfTranslation, pTf);
	if (m_pTfTranslation)
		m_pTfTranslation->AddRef();
	if (pTf)
		pTf->Release();
}

void CVeDtLrc::SetLyric(CLyric* pLrc)
{
	ECK_DUILOCK;
	std::swap(m_pLrc, pLrc);
	if (m_pLrc)
		m_pLrc->AddRef();
	if (pLrc)
		pLrc->Release();
}
