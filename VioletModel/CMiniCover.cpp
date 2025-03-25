#include "pch.h"
#include "CMiniCover.h"
#include "CWndMain.h"

constexpr static float CoverAnEndValue = 6.f;

LRESULT CMiniCover::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		float fValue;
		if (m_pec->IsActive())
		{
			fValue = m_pec->GetCurrValue();
		BlurDC:
			auto rcView{ GetViewRectF() };
			eck::InflateRect(rcView, fValue, fValue);
			m_pDC->DrawBitmap(m_pBmp, rcView);
			m_pDC->Flush();
			GetWnd()->CacheReserveLogSize(GetWidthF(), GetHeightF());
			GetWnd()->BlurDrawDC(GetRectInClientF(), {}, fValue);

			rcView = GetViewRectF();
			rcView.left = (rcView.right - (float)CxyPlayPageArrow) / 2;
			rcView.right = rcView.left + (float)CxyPlayPageArrow;
			rcView.top = (rcView.bottom - (float)CxyPlayPageArrow) / 2 +
				(CoverAnEndValue - fValue) * 4.f/*箭头的行程因子*/;
			rcView.bottom = rcView.top + (float)CxyPlayPageArrow;
			m_pDC->DrawBitmap(m_pBmpCoverUp, rcView, fValue / CoverAnEndValue);
		}
		else
		{
			if (m_bHover)
			{
				fValue = CoverAnEndValue;
				goto BlurDC;
			}
			else
				m_pDC->DrawBitmap(m_pBmp, GetViewRectF());
		}

		ECK_DUI_DBG_DRAW_FRAME;
		EndPaint(ps);
	}
	return 0;
	case WM_MOUSEMOVE:
	{
		if (!m_bHover)
		{
			m_bHover = TRUE;
			m_pec->SetReverse(FALSE);
			m_pec->Begin();
			GetWnd()->WakeRenderThread();
		}
	}
	return 0;
	case WM_MOUSELEAVE:
	{
		if (m_bHover)
		{
			m_bHover = FALSE;
			m_pec->SetReverse(TRUE);
			m_pec->Begin(eck::ECBF_CONTINUE);
			GetWnd()->WakeRenderThread();
		}
	}
	return 0;

	case WM_CREATE:
	{
		m_pec = new eck::CEasingCurve{};
		InitEasingCurve(m_pec);
		m_pec->SetAnProc(eck::Easing::OutCubic);
		m_pec->SetRange(0.f, CoverAnEndValue);
		m_pec->SetDuration(160.f);
		m_pec->SetCallBack([](float fCurrValue, float fOldValue, LPARAM lParam)
			{
				const auto p = (CMiniCover*)lParam;
				p->InvalidateRect();
			});

		GetWnd()->BmpNewLogSize(GetWidthF(), GetHeightF(), m_pBmp);
		const auto pOrg = ((CWndMain*)GetWnd())->RealizeImg(GImg::DefaultCover);
		m_pBmpCoverUp = ((CWndMain*)GetWnd())->RealizeImg(GImg::PlayPageUp);
		m_pBmpCoverUp->AddRef();

		ID2D1Image* pOldTarget;
		m_pDC->GetTarget(&pOldTarget);
		m_pDC->BeginDraw();
		m_pDC->SetTarget(m_pBmp);
		m_pDC->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pDC->DrawBitmap(pOrg, GetViewRectF());
		m_pDC->EndDraw();
		m_pDC->SetTarget(pOldTarget);
		if (pOldTarget)
			pOldTarget->Release();
	}
	break;

	case WM_DESTROY:
	{
		SafeRelease(m_pec);
		SafeRelease(m_pBmp);
		SafeRelease(m_pBmpCoverUp);
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}