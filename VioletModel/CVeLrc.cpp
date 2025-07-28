#include "pch.h"
#include "CVeLrc.h"

enum
{
	IDT_MOUSEIDLE = 1001,
	TE_MOUSEIDLE = 500,
	T_MOUSEIDLEMAX = 4500,
};

constexpr inline float AnDurLrcSelBkg{ 120.f };			// 歌词选中背景动画时长
constexpr inline float AnDurLrcScrollExpand{ 300.f };	// 滚动展开动画时长

static constexpr D2D1_COLOR_F InterpolateColor(
	const D2D1_COLOR_F& c1, const D2D1_COLOR_F& c2, float k)
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
	const auto iMs = p->m_psv->GetCurrTickInterval();
	if (p->m_AnEnlarge.IsEnd())
	{
		p->m_bEnlarging = FALSE;
		p->m_idxPrevAnItem = -1;
		p->m_fAnValue = p->m_fPlayingItemScale;
	}
	else
		p->m_fAnValue = p->m_AnEnlarge.Tick((float)iMs);

	EckAssert(AnDurLrcScrollExpand < (float)p->m_psv->GetDuration());
	if (p->m_bScrollExpand)
	{
		if (p->m_bSeEnlarging)
			p->m_msScrollExpand += iMs;
		else
			p->m_msScrollExpand -= iMs;
		p->m_kScrollExpand = eck::Easing::Linear(p->m_msScrollExpand, 1.f,
			p->m_fPlayingItemScale - 1.f, AnDurLrcScrollExpand);
		if (p->m_msScrollExpand >= AnDurLrcScrollExpand ||
			p->m_msScrollExpand <= 0.f)
		{
			p->m_bScrollExpand = FALSE;
			p->m_kScrollExpand = p->m_bSeEnlarging ? p->m_fPlayingItemScale : 1.f;
			p->m_msScrollExpand = p->m_bSeEnlarging ? AnDurLrcScrollExpand : 0.f;
		}
	}

	p->CalcTopItem();
	p->InvalidateRect();
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

float CVeLrc::DrawItem(int idx)
{
	EckAssert(idx >= 0 && idx < (int)m_vItem.size());
	auto& Item = m_vItem[idx];
	const auto y = Item.y - m_psv->GetPos();
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
	Mat.dx += m_cxyLineMargin;
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
		GetItemRect(idx, rc);
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
		if (m_bScrollExpand || MiIsIdle())
		{
			m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(
				m_kScrollExpand, m_kScrollExpand, ptScale) * Mat);
		}
		else
			m_pDC->SetTransform(Mat);
		m_pBrush->SetColor(m_Color[m_idxPrevCurr == idx ? CriHiLight : CriNormal]);
	}
	m_pDC1->DrawGeometryRealization(Item.pGr.Get(), m_pBrush);
	m_pDC->SetTransform(Mat0);
	return TRUE;
}

void CVeLrc::MiBeginDetect()
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

void CVeLrc::SeBeginExpand(BOOL bEnlarge)
{
	const auto bWake = !IsValid();
	if (!m_bScrollExpand)
	{
		m_bScrollExpand = TRUE;
		if (m_bSeEnlarging = bEnlarge)
			m_msScrollExpand = 0.f;
		else
			m_msScrollExpand = AnDurLrcScrollExpand;
	}
	if (bWake)
		GetWnd()->WakeRenderThread();
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
			for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
			{
				if (DrawItem(i) > GetHeightF())
					break;
			}
		ECK_DUI_DBG_DRAW_FRAME;
		EndPaint(ps);
	}
	return 0;

	case WM_MOUSEMOVE:
	{
		ECK_DUILOCK;
		if (m_vItem.empty())
			break;
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ClientToElem(pt);

		int idx = HitTest(pt);
		if (idx != m_idxHot)
		{
			std::swap(idx, m_idxHot);
			if (idx >= 0)
			{
				m_vItem[idx].OnKillHot();
				IsbWakeRenderThread();
			}
			if (m_idxHot >= 0)
			{
				m_vItem[m_idxHot].OnSetHot();
				IsbWakeRenderThread();
			}
		}
	}
	return 0;

	case WM_MOUSELEAVE:
	{
		ECK_DUILOCK;
		if (m_vItem.empty())
			break;
		int idx = -1;
		if (idx != m_idxHot)
		{
			std::swap(idx, m_idxHot);
			if (idx >= 0)
			{
				m_vItem[idx].OnKillHot();
				IsbWakeRenderThread();
			}
		}
	}
	return 0;

	case WM_MOUSEWHEEL:
	{
		ECK_DUILOCK;
		if (m_vItem.empty())
			break;
		if (!MiIsIdle())
			SeBeginExpand(TRUE);
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

	case WM_NOTIFY:
	{
		if ((Dui::CElem*)wParam == &m_SB)
		{
			switch (((Dui::DUINMHDR*)lParam)->uCode)
			{
			case Dui::EE_VSCROLL:
			{
				ECK_DUILOCK;
				if (!MiIsIdle())
					SeBeginExpand(TRUE);
				Scrolled();
				m_psv->InterruptAnimation();
				CalcTopItem();
				InvalidateRect();
			}
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
		LayoutItem();
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
		GetWnd()->RegisterTimeLine(this);

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
	break;

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
#ifdef _DEBUG
	if (idxCurr >= (int)m_vItem.size())
	{
		EckDbgPrintFmt(ECK_FUNCTIONW L"\n\t索引越界：%d", idxCurr);
		EckDbgBreak();
	}
#endif
	ECK_DUILOCK;
	const int idxPrev = m_idxPrevCurr;
	m_idxPrevCurr = idxCurr;
	if (MiIsIdle())
	{
		if (idxPrev >= 0)
			InvalidateItem(idxPrev);
		if (m_idxPrevCurr >= 0)
			InvalidateItem(m_idxPrevCurr);
	}
	else
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
	return S_OK;
}

HRESULT CVeLrc::LrcInit(std::shared_ptr<std::vector<eck::LRCINFO>> pvLrc)
{
	ECK_DUILOCK;
	m_pvLrc = pvLrc;
	m_idxPrevCurr = -1;
	LayoutItem();
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

void CVeLrc::Tick(int iMs)
{
	BOOL bAn{};
	EckCounter(m_vItem.size(), i)
	{
		auto& e = m_vItem[i];
		if (e.bAnSelBkg)
		{
			if (e.bAnSelBkgEnlarge)
				e.msAnSelBkg -= iMs;
			else
				e.msAnSelBkg += iMs;
			e.kAnSelBkg = eck::Easing::Linear(
				e.msAnSelBkg, 0.f, 1.f, AnDurLrcSelBkg);
			if (e.kAnSelBkg >= 1.f || e.kAnSelBkg <= 0.f)
				e.bAnSelBkg = FALSE;
			else
				bAn = TRUE;
			InvalidateItem(i);
		}
	}
	m_bAnSelBkg = bAn;
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
	SeBeginExpand(FALSE);
	GetWnd()->WakeRenderThread();
}

void CVeLrc::LayoutItem()
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
		e.cx += (m_cxyLineMargin * 2.f);
		e.cy += (m_cxyLineMargin * 2.f);
		y += (e.cy + m_cyLinePadding);
	}

	m_psv->SetMin(int(-cy / 3.f - m_vItem.front().cy));
	m_psv->SetMax(int(y - m_cyLinePadding + cy / 3.f * 2.f));
	m_psv->SetPage((int)cy);
}

void CVeLrc::Scrolled()
{
	MiBeginDetect();
	m_bEnlarging = FALSE;
	m_idxPrevAnItem = -1;
	m_fAnValue = m_fPlayingItemScale;
}


void CVeLrc::ITEM::OnSetHot()
{
	if (!bAnSelBkg)
	{
		bAnSelBkg = TRUE;
		msAnSelBkg = AnDurLrcSelBkg;
	}
	bAnSelBkgEnlarge = TRUE;
}

void CVeLrc::ITEM::OnKillHot()
{
	if (!bAnSelBkg)
	{
		bAnSelBkg = TRUE;
		msAnSelBkg = 0.f;
	}
	bAnSelBkgEnlarge = FALSE;
}