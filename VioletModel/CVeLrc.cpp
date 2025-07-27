#include "pch.h"
#include "CVeLrc.h"

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

static constexpr D2D1_COLOR_F InterpolateColor(const D2D1_COLOR_F& c1, const D2D1_COLOR_F& c2, float k)
{
	return {
		c1.r + (c2.r - c1.r) * k,
		c1.g + (c2.g - c1.g) * k,
		c1.b + (c2.b - c1.b) * k,
		c1.a + (c2.a - c1.a) * k };
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
	EckDbgPrint(p->m_idxTop);

	p->InvalidateRect();
}

void CVeLrc::FillItemBkg(int idx, const D2D1_RECT_F& rc)
{
	Dui::State eState;
	if (m_vItem[idx].bSel)
		if (idx == m_idxHot)
			eState = Dui::State::HotSelected;
		else
			eState = Dui::State::Selected;
	else
		if (idx == m_idxHot)
			eState = Dui::State::Hot;
		else
			return;
	GetTheme()->DrawBackground(Dui::Part::ListItem, eState, rc, nullptr);
}

void CVeLrc::CalcTopItem()
{
	if (m_vItem.empty())
	{
		m_idxTop = 0;
		return;
	}
	auto it = std::lower_bound(m_vItem.begin(), m_vItem.end(), (float)m_psv->GetPos(),
		[](const ITEM& r, float f) { return r.y < f; });
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
	D2D1_RECT_F rc;
	GetItemRect(idx, rc);
	y = rc.top;

	D2D1_MATRIX_3X2_F Mat0;
	m_pDC->GetTransform(&Mat0);

	D2D1_POINT_2F ptScale{};
	ptScale.y = Item.cy / 2.f - m_cxyLineMargin;
	switch (m_eAlignH)
	{
	case eck::Align::Center:
		ptScale.x = GetWidthF() / 2.f;
		break;
	case eck::Align::Far:
		ptScale.x = GetWidthF();
		break;
	}

	D2D1_MATRIX_3X2_F Mat{ Mat0 };
	const auto cyExtra = (Item.cy - m_cxyLineMargin * 2.f) *
		(m_fPlayingItemScale - 1.f) / 2.f;
	Mat.dy += (y + m_cxyLineMargin + cyExtra);

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

		ComPtr<ID2D1PathGeometry> pPathGeometry;
		eck::GetTextLayoutPathGeometry(&Item.pLayout, 2, cyPadding,
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

	FillItemBkg(idx, rc);
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
		m_pDC->SetTransform(Mat);
		m_pBrush->SetColor(m_Color[m_idxPrevCurr == idx ? CriHiLight : CriNormal]);
	}
	m_pDC1->DrawGeometryRealization(Item.pGr.Get(), m_pBrush);
	m_pDC->SetTransform(Mat0);
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

void CVeLrc::GetItemRect(int idx, _Out_ RECT& rc)
{
	const auto& e = m_vItem[idx];
	rc.left = (long)e.x;
	rc.top = (long)e.y - m_psv->GetPos();
	rc.right = long(e.x + e.cx);
	rc.bottom = long(rc.top + e.cy);
}

void CVeLrc::GetItemRect(int idx, _Out_ D2D1_RECT_F& rc)
{
	const auto& e = m_vItem[idx];
	rc.left = e.x;
	rc.top = e.y - m_psv->GetPos();
	rc.right = e.x + e.cx;
	rc.bottom = rc.top + e.cy;
}

void CVeLrc::InvalidateItem(int idx)
{
	D2D1_RECT_F rc;
	GetItemRect(idx, rc);
	ElemToClient(rc);
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
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			if (idx >= 0)
				m_idxMark = idx;
		}
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			if (m_idxMark < 0 || idx < 0)
				break;
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
	if (idxCurr < 0)
		return E_BOUNDS;
	if (m_idxPrevCurr == idxCurr)
		return S_FALSE;
	EckDbgPrint(idxCurr);
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
		const auto yDest = e.y + e.cy / 2.f;
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

void CVeLrc::LrcClear()
{
	ECK_DUILOCK;
	m_pvLrc.reset();
	m_vItem.clear();
	m_idxTop = -1;
	m_idxHot = -1;
	m_idxMark = -1;
	m_idxPrevCurr = -1;
	m_idxPrevAnItem = -1;
	m_idxCurrAnItem = -1;
	m_fAnValue = 1.f;
	m_bEnlarging = FALSE;
	InvalidateRect();
}

void CVeLrc::SetTextFormatTrans(IDWriteTextFormat* pTf)
{
	ECK_DUILOCK;
	std::swap(m_pTextFormatTrans, pTf);
	if (m_pTextFormatTrans)
		m_pTextFormatTrans->AddRef();
	if (pTf)
		pTf->Release();
	CallEvent(WM_SETFONT, 0, 0);
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
	float yDest = CurrItem.y + CurrItem.cy / 2.f;
	m_psv->InterruptAnimation();
	m_psv->SmoothScrollDelta(int((yDest - GetHeight() / 3) - m_psv->GetPos()));
	GetWnd()->WakeRenderThread();
}

void CVeLrc::LayoutItems()
{
	m_vItem.clear();
	if (!m_pvLrc || m_pvLrc->empty())
		return;
	const float cx = GetWidthF(), cy = GetHeightF();
	if (cx <= 0.f || cy <= 0.f)
		return;
	const auto& vLrc = *m_pvLrc;

	constexpr auto cyMainTransPadding = 5.f;

	m_vItem.resize(vLrc.size());
	const float cxMax = cx / m_fPlayingItemScale;
	DWRITE_TEXT_METRICS Metrics;
	float y = 0.f;
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
		e.cx *= m_fPlayingItemScale;
		e.cy *= m_fPlayingItemScale;
		e.cy += (m_cxyLineMargin * 2.f);
		y += (e.cy + m_cyLinePadding);
	}

	m_psv->SetMin(int(-cy / 3.f - m_vItem.front().cy));
	m_psv->SetMax(int(y - m_cyLinePadding + cy / 3.f * 2.f));
	m_psv->SetPage((int)cy);
}

void CVeLrc::Scrolled()
{
	BeginMouseIdleDetect();
	m_bEnlarging = FALSE;
	m_idxPrevAnItem = -1;
	m_fAnValue = m_fPlayingItemScale;
}