#include "pch.h"
#include "CVeDtLrc.h"
#include "CApp.h"


void CVeDtLrc::InvalidateCache()
{
	for (auto& e : m_Line)
		e.idxLrc = c_InvalidCacheIdx;
}

float CVeDtLrc::DrawLrcLine(int idxLrc, float y, BOOL bSecondLine)
{
	EckAssert(bSecondLine == 1 || bSecondLine == 0);
	const auto pLyric = GetLyric();
	auto& e = m_Line[bSecondLine];
	const BOOL bHiLight = (idxLrc == m_idxCurr);

	const float cxMax = GetWidthF();
	const float cyMax = GetHeightF();

	if (e.idxLrc != idxLrc)// 更新缓存
	{
		const auto bOldTooLong = m_bTooLong;
		const auto& Lrc = pLyric->MgAtLine(idxLrc);
		float xDpi, yDpi;
		m_pDC1->GetDpi(&xDpi, &yDpi);
		const float fTolerance = D2D1::ComputeFlatteningTolerance(
			D2D1::Matrix3x2F::Identity(), xDpi, yDpi, 1.f);

		e.idxLrc = idxLrc;
		SafeRelease(e.pLayout);
		SafeRelease(e.pLayoutTrans);

		//-----------重建文本布局
		if (Lrc.cchLrc)
			eck::g_pDwFactory->CreateTextLayout(Lrc.pszLrc, Lrc.cchLrc,
				GetTextFormat(), cxMax, cyMax, &e.pLayout);
		else
			eck::g_pDwFactory->CreateTextLayout(EckStrAndLen(L"♪♪♪"),
				GetTextFormat(), cxMax, cyMax, &e.pLayout);
		DWRITE_TEXT_METRICS tm;
		e.pLayout->GetMetrics(&tm);
		e.bTooLong = (tm.width > cxMax);
		e.size = { tm.width,tm.height };
		//-----------重建几何实现
		ComPtr<ID2D1PathGeometry1> pPath;
		eck::GetTextLayoutPathGeometry(e.pLayout, m_pDC1, 0, 0, pPath.RefOfClear());
		SafeRelease(e.pGrF);
		SafeRelease(e.pGrS);
		m_pDC1->CreateFilledGeometryRealization(pPath.Get(), fTolerance, &e.pGrF);
		m_pDC1->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
			m_cxOutline, nullptr, &e.pGrS);
		if (Lrc.cchTranslation)
		{
			//-----------重建文本布局
			eck::g_pDwFactory->CreateTextLayout(Lrc.pszTranslation, Lrc.cchTranslation,
				GetTextFormatTrans(), cxMax, cyMax, &e.pLayoutTrans);
			e.pLayoutTrans->GetMetrics(&tm);
			e.bTooLongTrans = (tm.width > cxMax);
			e.sizeTrans = { tm.width,tm.height };
			//-----------重建几何实现
			eck::GetTextLayoutPathGeometry(e.pLayoutTrans, m_pDC1, 0, 0, pPath.RefOfClear());
			SafeRelease(e.pGrFTrans);
			SafeRelease(e.pGrSTrans);
			m_pDC1->CreateFilledGeometryRealization(pPath.Get(), fTolerance, &e.pGrFTrans);
			m_pDC1->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
				m_cxOutline, nullptr, &e.pGrSTrans);
		}
		m_bTooLong = !!e.bTooLong || !!e.bTooLongTrans;
	}
	const auto& Lrc = pLyric->MgAtLine(idxLrc);
	NM_DTL_GET_TIME nm{ ELEN_DTLRC_GET_TIME };
	GenElemNotify(&nm);

	const float cxMaxHalf = cxMax / 2.f;
	float dx;
	if (e.bTooLong)
	{
		dx = (nm.fTime - Lrc.fTime) * e.size.width / Lrc.fDuration;
		if (dx < cxMaxHalf)
			dx = 0.f;
		else if (dx > e.size.width - cxMaxHalf)
			dx = cxMax - e.size.width;
		else
			dx = cxMaxHalf - dx;
	}
	else
		switch (m_eAlign[bSecondLine])
		{
		case eck::Align::Near:	dx = 0.f; break;
		case eck::Align::Center:dx = (cxMax - e.size.width) / 2.f; break;
		case eck::Align::Far:	dx = cxMax - e.size.width; break;
		default: ECK_UNREACHABLE;
		}
	DrawTextGeometry(e.pGrS, e.pGrF, dx, y,
		m_pBrush[bHiLight ? BriMainHiLight : BriMain]);

	float cy = e.size.height;
	if (e.pLayoutTrans)
	{
		const float yNew = y + cy;
		if (e.bTooLongTrans)
		{
			dx = (nm.fTime - Lrc.fTime) * e.sizeTrans.width / Lrc.fDuration;
			if (dx < cxMaxHalf)
				dx = 0;
			else if (dx > e.sizeTrans.width - cxMaxHalf)
				dx = cxMax - e.sizeTrans.width;
			else
				dx = cxMaxHalf - dx;
		}
		else
			switch (m_eAlign[bSecondLine])
			{
			case eck::Align::Near:	dx = 0.f; break;
			case eck::Align::Center:dx = (cxMax - e.sizeTrans.width) / 2.f; break;
			case eck::Align::Far:	dx = cxMax - e.sizeTrans.width; break;
			default: ECK_UNREACHABLE;
			}
		DrawTextGeometry(e.pGrSTrans, e.pGrFTrans, dx, yNew,
			m_pBrush[bHiLight ? BriTransHiLight : BriTrans]);
		cy += e.sizeTrans.height;
	}
	return cy;
}

void CVeDtLrc::DrawStaticLine(float y)
{
	auto& e = m_StaticLine;
	if (!e.bValid)
	{
		e.bValid = TRUE;
		float xDpi, yDpi;
		m_pDC1->GetDpi(&xDpi, &yDpi);
		const float fTolerance = D2D1::ComputeFlatteningTolerance(
			D2D1::Matrix3x2F::Identity(), xDpi, yDpi, 1.f);
		//-----------重建文本布局
		ComPtr<IDWriteTextLayout> pLayout;
		eck::g_pDwFactory->CreateTextLayout(e.rsText.Data(),
			e.rsText.Size(), GetTextFormat(),
			GetWidthF(), GetHeightF(), &pLayout);
		DWRITE_TEXT_METRICS tm;
		pLayout->GetMetrics(&tm);
		e.cx = tm.width;
		//-----------重建几何实现
		ComPtr<ID2D1PathGeometry1> pPath;
		eck::GetTextLayoutPathGeometry(pLayout.Get(), m_pDC1, 0, 0, pPath.RefOf());
		SafeRelease(e.pGrF);
		SafeRelease(e.pGrS);
		m_pDC1->CreateFilledGeometryRealization(pPath.Get(), fTolerance, &e.pGrF);
		m_pDC1->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
			m_cxOutline, nullptr, &e.pGrS);
	}
	DrawTextGeometry(e.pGrS, e.pGrF,
		(GetWidthF() - e.cx) / 2.f, y, m_pBrush[BriMain]);
}

void CVeDtLrc::DrawTextGeometry(ID2D1GeometryRealization* pGrS,
	ID2D1GeometryRealization* pGrF, float dx, float dy, ID2D1Brush* pBrFill)
{
	D2D1_MATRIX_3X2_F Mat0;
	m_pDC1->GetTransform(&Mat0);
	auto Mat{ Mat0 };
	Mat.dx += dx;
	Mat.dy += dy;
	if (pGrF && m_bShadow)
	{
		Mat.dx += m_dxyShadow;
		Mat.dy += m_dxyShadow;
		m_pDC1->SetTransform(Mat);
		m_pDC1->DrawGeometryRealization(pGrF, m_pBrush[BriShadow]);
		Mat.dx -= m_dxyShadow;
		Mat.dy -= m_dxyShadow;
	}
	m_pDC1->SetTransform(Mat);
	if (pGrS)
		m_pDC1->DrawGeometryRealization(pGrS, m_pBrush[BriBorder]);
	if (pGrF)
		m_pDC1->DrawGeometryRealization(pGrF, pBrFill);
	m_pDC1->SetTransform(Mat0);
}

LRESULT CVeDtLrc::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		if (m_idxCurr >= 0)
		{
			float y{}, cy;
			if (m_idxCurr % 2)
			{
				cy = DrawLrcLine(m_idxCurr, y, FALSE);
				if (m_idxCurr + 1 < GetLyric()->MgGetLineCount())
					DrawLrcLine(m_idxCurr + 1, y + cy + m_cyLinePadding, TRUE);
			}
			else
			{
				int idx = m_idxCurr + 1;
				if (idx >= GetLyric()->MgGetLineCount())
					idx = m_idxCurr - 1;

				if (idx >= 0 && idx < GetLyric()->MgGetLineCount())
					cy = DrawLrcLine(idx, y, FALSE);
				else
					cy = 0.f;
				DrawLrcLine(m_idxCurr, y + cy + m_cyLinePadding, TRUE);
			}
		}
		else
			DrawStaticLine(0.f);

		ECK_DUI_DBG_DRAW_FRAME;
		EndPaint(ps);
	}
	return 0;
	case WM_CREATE:
	{
		GetWnd()->RegisterTimeLine(this);

		m_pDC->QueryInterface(&m_pDC1);
		InvalidateCache();

		ComPtr<ID2D1SolidColorBrush> pBr;
		m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBr);
		m_pBrush[BriBorder] = pBr.Detach();

		m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBr);
		m_pBrush[BriMain] = pBr.Detach();

		m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBr);
		m_pBrush[BriMainHiLight] = pBr.Detach();

		m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pBr);
		m_pBrush[BriTrans] = pBr.Detach();

		m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &pBr);
		m_pBrush[BriTransHiLight] = pBr.Detach();

		m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 0.4f), &pBr);
		m_pBrush[BriShadow] = pBr.Detach();
	}
	break;
	case WM_DESTROY:
	{
		for (auto& e : m_pBrush)
			SafeRelease(e);
		for (auto& e : m_Line)
		{
			SafeRelease(e.pGrS);
			SafeRelease(e.pGrSTrans);
			SafeRelease(e.pGrF);
			SafeRelease(e.pGrFTrans);
			SafeRelease(e.pLayout);
			SafeRelease(e.pLayoutTrans);
		}
		SafeRelease(m_pDC1);
		SafeRelease(m_StaticLine.pGrS);
		SafeRelease(m_StaticLine.pGrF);
		SafeRelease(m_pLrc);
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeDtLrc::Tick(int iMs)
{
	if (m_bTooLong)
	{
		InvalidateRect();
	}
}

HRESULT CVeDtLrc::LrcSetCurrentLine(int idx)
{
	if (m_idxCurr == idx)
		return S_FALSE;
	m_idxCurr = idx;
	InvalidateCache();
	InvalidateRect();
	return S_OK;
}

void CVeDtLrc::LrcSetEmptyText(std::wstring_view svEmptyText)
{
	ECK_DUILOCK;
	m_StaticLine.rsText = svEmptyText;
	m_StaticLine.bValid = FALSE;
}

void CVeDtLrc::SetTextFormatTrans(IDWriteTextFormat* pTf)
{
	ECK_DUILOCK;
	std::swap(m_pTfTranslation, pTf);
	if (m_pTfTranslation)
		m_pTfTranslation->AddRef();
	if (pTf)
		pTf->Release();
}

void CVeDtLrc::SetLyric(Lyric::CLyric* pLrc)
{
	ECK_DUILOCK;
	std::swap(m_pLrc, pLrc);
	if (m_pLrc)
		m_pLrc->AddRef();
	if (pLrc)
		pLrc->Release();
}