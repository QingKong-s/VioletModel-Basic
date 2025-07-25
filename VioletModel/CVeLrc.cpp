#include "pch.h"
#include "CVeLrc.h"
#include "CApp.h"

enum
{
	IDT_MOUSEIDLE = 1001,
	TE_MOUSEIDLE = 500,
	T_MOUSEIDLEMAX = 4500,
};

template <class FwdIt, class Ty, class Pr>
[[nodiscard]] constexpr static FwdIt LowerBound(FwdIt First, const FwdIt Last, const Ty& Val, Pr Pred) {

	auto UFirst = (First);
	std::iter_difference_t<FwdIt> Count = std::distance(UFirst, (Last));

	while (0 < Count) {
		const std::iter_difference_t<FwdIt> Count2 = Count / 2;
		const auto UMid = std::next(UFirst, Count2);
		if (Pred(UMid, Val)) {
			UFirst = std::next(UMid);
			Count -= Count2 + 1;
		}
		else
			Count = Count2;
	}

	return UFirst;
}

void CVeLrc::ScrollProc(int iPos, int iPrevPos, LPARAM lParam)
{
	const auto p = (CVeLrc*)lParam;
	if (p->m_AnEnlarge.IsEnd())
	{
		p->m_bEnlarging = FALSE;
		p->m_idxPrevAnItem = -1;
		p->m_fAnValue = p->m_fPlayingItemScale;
	}
	else
		p->m_fAnValue = p->m_AnEnlarge.Tick(
			(float)p->m_psv->GetCurrTickInterval());

	p->CalcTopItem();
	p->InvalidateRect();
}

void CVeLrc::FillItemBkg(int idx, const D2D1_RECT_F& rc)
{
	D2D1_COLOR_F cr;
	if (m_vItem[idx].bSel)
		if (idx == m_idxHot)
			GetTheme()->GetColor(Dui::Part::ListItem, Dui::State::HotSelected,
				eck::ClrPart::Bk, cr);
		else
			GetTheme()->GetColor(Dui::Part::ListItem, Dui::State::Selected,
				eck::ClrPart::Bk, cr);
	else
		if (idx == m_idxHot)
			GetTheme()->GetColor(Dui::Part::ListItem, Dui::State::Selected,
				eck::ClrPart::Bk, cr);
		else
			return;
	m_pBrush->SetColor(cr);
	m_pDC->FillRectangle(rc, m_pBrush);
}

void CVeLrc::CalcTopItem()
{
	if (m_vItem.empty())
	{
		m_idxTop = 0;
		return;
	}
	auto it = LowerBound(m_vItem.begin(), m_vItem.end(), m_psv->GetPos(),
		[this](decltype(m_vItem)::iterator it, int iPos)
		{
			const auto& e = *it;
			float y = e.y;
			const int idx = (int)std::distance(m_vItem.begin(), it);
			if (idx > m_idxPrevAnItem && m_idxPrevAnItem >= 0)
				y += (m_vItem[m_idxPrevAnItem].cy * (m_fPlayingItemScale - m_fAnValue));
			if (idx > m_idxCurrAnItem && m_idxCurrAnItem >= 0)
				y += (m_vItem[m_idxCurrAnItem].cy * (m_fAnValue - 1.f));
			return y < iPos;
		});
	EckAssert(it != m_vItem.end());

	if (it == m_vItem.begin())
	{
		m_idxTop = 0;
		return;
	}
	--it;
	m_idxTop = (int)std::distance(m_vItem.begin(), it);
}

BOOL CVeLrc::DrawItem(int idx, float& y)
{
	EckAssert(idx >= 0 && idx < (int)m_vItem.size());
	auto& Item = m_vItem[idx];
	y = GetItemY(idx);
	const D2D1_RECT_F rc{ Item.x,0,Item.x + Item.cx,Item.cy };

	D2D1_MATRIX_3X2_F mat0;
	m_pDC->GetTransform(&mat0);

	D2D1_POINT_2F ptScale{};
	switch (m_eAlignH)
	{
	case eck::Align::Center:
		ptScale.x = GetWidthF() / 2.f;
		break;
	case eck::Align::Far:
		ptScale.x = GetWidthF();
		break;
	}

	D2D1_MATRIX_3X2_F mat = mat0 * D2D1::Matrix3x2F::Translation(0, y);

	if (!Item.bCacheValid)
	{
		constexpr float cyPadding[]{ 5.f,0.f };

		float x[2]{};
		switch (m_eAlignH)
		{
		case eck::Align::Center:
			x[0] = (GetWidthF() - GetWidthF() / m_fPlayingItemScale) / 2.f;
			x[1] = (GetWidthF() - Item.cxTrans) / 2.f;
			break;
		case eck::Align::Far:
			x[0] = (GetWidthF() - GetWidthF() / m_fPlayingItemScale);
			x[1] = (GetWidthF() - Item.cxTrans);
			break;
		}

		ID2D1PathGeometry* pPathGeometry;
		eck::GetTextLayoutPathGeometry(&Item.pLayout, 2, cyPadding, m_pDC, x, 0.f, pPathGeometry);
		float xDpi, yDpi;
		m_pDC->GetDpi(&xDpi, &yDpi);

		ID2D1GeometryRealization* pGr = nullptr;
		m_pDC1->CreateFilledGeometryRealization(
			pPathGeometry,
			D2D1::ComputeFlatteningTolerance(
				D2D1::Matrix3x2F::Identity(), xDpi, yDpi, m_fPlayingItemScale),
			&pGr);

		if (Item.pGr)
			Item.pGr->Release();
		Item.pGr = pGr;
		pPathGeometry->Release();
		Item.bCacheValid = TRUE;
	}

	if (idx == m_idxPrevAnItem)
	{
		m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(
			m_fPlayingItemScale + 1.f - m_fAnValue,
			m_fPlayingItemScale + 1.f - m_fAnValue,
			ptScale) * mat);
		if (Item.bSel || idx == m_idxHot)
			FillItemBkg(idx, rc);
		m_pDC1->DrawGeometryRealization(Item.pGr, m_pBrush);
		m_pDC->SetTransform(mat0);
		return TRUE;
	}

	if (idx == m_idxCurrAnItem)
	{
		m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(
			m_fAnValue,
			m_fAnValue,
			ptScale) * mat);
		if (Item.bSel || idx == m_idxHot)
			FillItemBkg(idx, rc);
		m_pDC1->DrawGeometryRealization(Item.pGr, m_pBrush);
		m_pDC->SetTransform(mat0);
		return TRUE;
	}

	m_pDC->SetTransform(mat);
	FillItemBkg(idx, rc);
	m_pDC1->DrawGeometryRealization(Item.pGr, m_pBrush);
	m_pDC->SetTransform(mat0);
	return TRUE;
}

void CVeLrc::BeginMouseIdleDetect()
{
	if (!m_tMouseIdle)
		SetTimer(IDT_MOUSEIDLE, TE_MOUSEIDLE);
	m_tMouseIdle = T_MOUSEIDLEMAX;
}

int CVeLrc::HitTest(POINT pt)
{
	RECT rc;
	for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
	{
		GetItemRect(i, rc);
		if (eck::PtInRect(rc, pt))
			return i;
		if (rc.top > GetHeight())
			break;
	}
	return -1;
}

void CVeLrc::GetItemRect(int idx, RECT& rc)
{
	auto& e = m_vItem[idx];
	const auto y = GetItemY(idx);
	rc.left = (long)e.x;
	rc.top = (long)y;
	if (idx == m_idxPrevAnItem)
	{
		const auto cx = (long)(e.cx * (m_fPlayingItemScale + 1.f - m_fAnValue));
		switch (m_eAlignH)
		{
		case eck::Align::Center:
			rc.left = (GetWidth() - cx) / 2;
			break;
		case eck::Align::Far:
			rc.left = GetWidth() - cx;
			break;
		}
		rc.right = rc.left + cx;
		rc.bottom = (long)(y + e.cy * (m_fPlayingItemScale + 1.f - m_fAnValue));
	}
	else if (idx == m_idxCurrAnItem)
	{
		const auto cx = (long)(e.cx * m_fAnValue);
		switch (m_eAlignH)
		{
		case eck::Align::Center:
			rc.left = (GetWidth() - cx) / 2;
			break;
		case eck::Align::Far:
			rc.left = GetWidth() - cx;
			break;
		}
		rc.right = rc.left + cx;
		rc.bottom = (long)(y + e.cy * m_fAnValue);
	}
	else ECKLIKELY
	{
		rc.right = rc.left + (long)e.cx;
		rc.bottom = (long)(y + e.cy);
	}
}

float CVeLrc::GetItemY(int idx)
{
	float y = m_vItem[idx].y - m_psv->GetPos();
	if (idx > m_idxPrevAnItem && m_idxPrevAnItem >= 0)
		y += (m_vItem[m_idxPrevAnItem].cy * (m_fPlayingItemScale - m_fAnValue));
	if (idx > m_idxCurrAnItem && m_idxCurrAnItem >= 0)
		y += (m_vItem[m_idxCurrAnItem].cy * (m_fAnValue - 1.f));
	return y;
}

void CVeLrc::InvalidateItem(int idx)
{
	RECT rc;
	GetItemRect(idx, rc);
	ElemToClient(rc);
	rc.top -= 1;
	rc.bottom += 1;
	rc.right += 1;
	InvalidateRect(rc);
}

LRESULT CVeLrc::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);

		if (m_vItem.empty())
		{
			if (m_pGrEmptyText)
				m_pDC1->DrawGeometryRealization(m_pGrEmptyText, m_pBrush);
		}
		else
		{
			float y;
			for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
			{
				DrawItem(i, y);
				if (y > GetHeightF())
					break;
			}
		}

		ECK_DUI_DBG_DRAW_FRAME;
		EndPaint(ps);
	}
	return 0;

	case WM_MOUSEMOVE:
	{
		if (m_vItem.empty())
			break;
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ClientToElem(pt);

		int idx = HitTest(pt);
		if (idx != m_idxHot)
		{
			ECK_DUILOCK;
			std::swap(idx, m_idxHot);
			SetRedraw(FALSE);
			if (idx >= 0)
				InvalidateItem(idx);
			if (m_idxHot >= 0)
				InvalidateItem(m_idxHot);
			SetRedraw(TRUE);
		}
	}
	return 0;

	case WM_MOUSELEAVE:
	{
		if (m_vItem.empty())
			break;
		int idx = -1;
		if (idx != m_idxHot)
		{
			ECK_DUILOCK;
			std::swap(idx, m_idxHot);
			if (idx >= 0)
				InvalidateItem(idx);
		}
	}
	return 0;

	case WM_MOUSEWHEEL:
	{
		if (m_vItem.empty())
			break;
		ECK_DUILOCK;
		Scrolled();
		m_psv->OnMouseWheel2(-GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
		GetWnd()->WakeRenderThread();
	}
	return 0;

	case WM_LBUTTONDOWN:
	{
		ECK_DUILOCK;
		SetFocus();
		if (m_vItem.empty())
			break;
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ClientToElem(pt);

		int idx = HitTest(pt);
		SetRedraw(FALSE);
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			if (idx >= 0)
				m_idxMark = idx;
		}
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			if (m_idxMark < 0 || idx < 0)
			{
				SetRedraw(TRUE);
				break;
			}
			const int idx0 = std::min(m_idxMark, idx);
			const int idx1 = std::max(m_idxMark, idx);
			int i = 0;
			for (; i < idx0; ++i)
			{
				if (m_vItem[i].bSel)
				{
					m_vItem[i].bSel = FALSE;
					InvalidateItem(i);
				}
			}
			for (; i <= idx1; ++i)
			{
				if (!m_vItem[i].bSel)
				{
					m_vItem[i].bSel = TRUE;
					InvalidateItem(i);
				}
			}
			for (; i < (int)m_vItem.size(); ++i)
			{
				if (m_vItem[i].bSel)
				{
					m_vItem[i].bSel = FALSE;
					InvalidateItem(i);
				}
			}
			SetRedraw(TRUE);
			break;
		}
		else
		{
			if (idx >= 0)
				m_idxMark = idx;
			EckCounter((int)m_vItem.size(), i)
			{
				if (m_vItem[i].bSel)
				{
					m_vItem[i].bSel = FALSE;
					InvalidateItem(i);
				}
			}
		}

		if (idx >= 0)
			if (!m_vItem[idx].bSel)
			{
				m_vItem[idx].bSel = TRUE;
				InvalidateItem(idx);
			}
		SetRedraw(TRUE);
	}
	return 0;

	case WM_KEYDOWN:
	{
		if (m_vItem.empty())
			break;
		if (wParam == VK_ESCAPE)
		{
			m_tMouseIdle = 0;
			KillTimer(IDT_MOUSEIDLE);
			ScrollToCurrPos();
		}
	}
	return 0;

	case WM_LBUTTONUP:
	{
		if (m_vItem.empty())
			break;
	}
	return 0;

	case WM_NOTIFY:
	{
		if ((Dui::CElem*)wParam == &m_SB)
		{
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::EE_VSCROLL:
				ECK_DUILOCK;
				Scrolled();
				m_psv->InterruptAnimation();
				CalcTopItem();
				InvalidateRect();
				return TRUE;
			}
		}
	}
	return 0;

	case WM_TIMER:
	{
		if (wParam == IDT_MOUSEIDLE)
		{
			ECK_DUILOCK;
			if (m_vItem.empty())
				break;
			m_tMouseIdle -= TE_MOUSEIDLE;
			if (m_tMouseIdle <= 0)
			{
				m_tMouseIdle = 0;
				KillTimer(IDT_MOUSEIDLE);
				ScrollToCurrPos();
			}
		}
	}
	return 0;

	case WM_SIZE:
	{
		LayoutItems();
		RECT rc;
		rc.left = GetWidth() - m_SB.GetWidth();
		rc.top = 0;
		rc.right = rc.left + m_SB.GetWidth();
		rc.bottom = rc.top + GetHeight();
		m_SB.SetRect(rc);
	}
	break;

	case WM_CREATE:
	{
		m_pDC->QueryInterface(&m_pDC1);
		m_pDC->CreateSolidColorBrush({}, &m_pBrush);

		m_SB.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, (int)GetTheme()->GetMetrics(Dui::Metrics::CxVScroll), 0,
			this, GetWnd());
		m_psv = m_SB.GetScrollView();
		m_psv->AddRef();
		m_psv->SetMinThumbSize(Dui::CxyMinScrollThumb);
		m_psv->SetCallBack(ScrollProc, (LPARAM)this);
		m_psv->SetDelta(80);
	}
	return 0;

	case WM_DESTROY:
	{
		SafeRelease(m_pBrush);
		SafeRelease(m_pTextFormat);
		SafeRelease(m_psv);
		SafeRelease(m_pDC1);
		SafeRelease(m_pGrEmptyText);
		m_vItem.clear();
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}

HRESULT CVeLrc::UpdateEmptyText(std::wstring_view svEmptyText)
{
	ECK_DUILOCK;
	HRESULT hr;
	SafeRelease(m_pGrEmptyText);

	auto& Player = App->GetPlayer();
	ComPtr<IDWriteTextLayout> pLayout;
	hr = eck::g_pDwFactory->CreateTextLayout(svEmptyText.data(),
		(UINT32)svEmptyText.size(), m_pTextFormat,
		GetWidthF(), GetHeightF(), &pLayout);
	if (FAILED(hr)) return hr;
	pLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

	ComPtr<ID2D1PathGeometry1> pPathGeometry;
	eck::GetTextLayoutPathGeometry(pLayout.Get(), m_pDC,
		0.f, 0.f, pPathGeometry.RefOf());
	float xDpi, yDpi;
	m_pDC->GetDpi(&xDpi, &yDpi);

	return m_pDC1->CreateFilledGeometryRealization(
		pPathGeometry.Get(),
		D2D1::ComputeFlatteningTolerance(
			D2D1::Matrix3x2F::Identity(), xDpi, yDpi),
		&m_pGrEmptyText);
}

HRESULT CVeLrc::LrcTick(int idxCurr)
{
	if (m_idxPrevCurr == idxCurr)
		return S_FALSE;
	ECK_DUILOCK;
	const int idxPrev = m_idxPrevCurr;
	m_idxPrevCurr = idxCurr;
	if (m_tMouseIdle <= 0)// 未手动滚动
	{
		m_bEnlarging = TRUE;
		m_idxPrevAnItem = idxPrev;
		m_idxCurrAnItem = m_idxPrevCurr;
		m_AnEnlarge.Begin(1.f, m_fPlayingItemScale - 1.f, (float)m_psv->GetDuration());
		const auto& e = m_vItem[m_idxPrevCurr < 0 ? 0 : m_idxPrevCurr];
		const auto yDest = e.y + e.cy * m_fPlayingItemScale / 2.f;
		m_psv->InterruptAnimation();
		m_psv->SmoothScrollDelta(int((yDest - GetHeightF() / 3.f) - m_psv->GetPos()));
		GetWnd()->WakeRenderThread();
	}
	else
	{
		if (idxPrev >= 0)
			InvalidateItem(idxPrev);
		if (m_idxPrevCurr >= 0)
			InvalidateItem(m_idxPrevCurr);
	}
	return S_OK;
}

HRESULT CVeLrc::LrcInit(std::shared_ptr<std::vector<eck::LRCINFO>> pvLrc)
{
	ECK_DUILOCK;
	m_pvLrc = pvLrc;
	m_idxPrevCurr = -1;
	LayoutItems();
	m_psv->SetPos(m_psv->GetMin());
	CalcTopItem();
	InvalidateRect();
	return S_OK;
}

void CVeLrc::ScrollToCurrPos()
{
	if (m_idxPrevCurr != m_idxCurrAnItem && m_idxCurrAnItem >= 0)
	{
		m_idxPrevAnItem = m_idxCurrAnItem;
		m_idxCurrAnItem = m_idxPrevCurr;
		m_bEnlarging = TRUE;
		m_AnEnlarge.Begin(1.f, m_fPlayingItemScale - 1.f, 400);
	}
	const auto& CurrItem = m_idxPrevCurr < 0 ? m_vItem.front() : m_vItem[m_idxPrevCurr];
	float yDest = CurrItem.y + CurrItem.cy * m_fPlayingItemScale / 2.f;
	m_psv->InterruptAnimation();
	m_psv->SmoothScrollDelta(int((yDest - GetHeight() / 3) - m_psv->GetPos()));
	GetWnd()->WakeRenderThread();
}

void CVeLrc::LayoutItems()
{
	auto& Player = App->GetPlayer();
	const auto& vLrc = *m_pvLrc;
	const float cx = GetWidthF(), cy = GetHeightF();

	m_vItem.clear();
	if (cx <= 0.f || cy <= 0.f || vLrc.empty())
		return;

	constexpr auto cyMainTransPadding = 5.f;

	m_vItem.resize(vLrc.size());
	const float cxMax = cx / m_fPlayingItemScale;
	DWRITE_TEXT_METRICS Metrics;
	float y = 0.f;
	const float cyPadding = 2.f;

	EckCounter(vLrc.size(), i)
	{
		auto& e = m_vItem[i];
		eck::g_pDwFactory->CreateTextLayout(vLrc[i].pszLrc, vLrc[i].cchLrc,
			m_pTextFormat, cxMax, cy, &e.pLayout);
		e.pLayout->GetMetrics(&Metrics);
		e.y = y;
		e.cx = Metrics.width;
		e.cy = Metrics.height;

		if (vLrc[i].pszTranslation)
		{
			eck::g_pDwFactory->CreateTextLayout(vLrc[i].pszTranslation,
				vLrc[i].cchTotal - vLrc[i].cchLrc,
				m_pTextFormatTrans, cxMax, cy, &e.pLayoutTrans);
			e.pLayoutTrans->GetMetrics(&Metrics);
			e.cxTrans = Metrics.width;
			e.cx = std::max(e.cx, Metrics.width);
			e.cy += (Metrics.height + cyMainTransPadding);
		}

		switch (m_eAlignH)
		{
		case eck::Align::Center:
			e.x = (cx - e.cx) / 2.f;
			break;
		case eck::Align::Far:
			e.x = cx - e.cx;
			break;
		}

		y += (e.cy + cyPadding);
	}

	m_psv->SetMin(int(-cy / 3.f - m_vItem.front().cy));
	m_psv->SetMax(int(y - cyPadding + cy / 3.f * 2.f));
	m_psv->SetPage((int)cy);
}

void CVeLrc::Scrolled()
{
	BeginMouseIdleDetect();
	m_bEnlarging = FALSE;
	m_idxPrevAnItem = -1;
	m_fAnValue = m_fPlayingItemScale;
}