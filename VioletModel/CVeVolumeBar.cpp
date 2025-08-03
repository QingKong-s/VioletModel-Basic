#include "pch.h"
#include "CVeVolumeBar.h"
#include "CApp.h"

LRESULT CVeVolumeBar::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		m_pBrush->SetColor(App->GetColor(GPal::VolBarBk));
		m_pDC->FillRectangle(ps.rcfClipInElem, m_pBrush);

		m_pBrush->SetColor(App->GetColor(GPal::VolBarBorder));
		m_pDC->DrawRectangle(GetViewRectF(), m_pBrush);
		EndPaint(ps);
	}
	return 0;
	case WM_SETFONT:
		m_LAVol.SetTextFormat(GetTextFormat());
		break;
	case WM_CREATE:
	{
		m_pecShowing = new eck::CEasingCurve{};
		InitEasingCurve(m_pecShowing);
		m_pecShowing->SetRange(0.f, 1.f);
		m_pecShowing->SetDuration(300);
		m_pecShowing->SetAnProc(eck::Easing::OutCubic);
		m_pecShowing->SetCallBack([](float fCurrValue, float fOldValue, LPARAM lParam)
			{
				const auto pThis = (CVeVolumeBar*)lParam;
				pThis->m_pPageAn->Opacity = fCurrValue;
				pThis->m_pPageAn->Dy = int((1.f - fCurrValue) * (float)DVolAn);
				pThis->CompReCalcCompositedRect();
				pThis->InvalidateRect();
				if (pThis->m_pecShowing->IsStop())
				{
					pThis->SetCompositor(nullptr);
					if (!pThis->m_bShow)
						pThis->SetStyle(pThis->GetStyle() & ~Dui::DES_VISIBLE);
				}
			});
		m_pPageAn = new Dui::CCompositorPageAn{};
		m_pPageAn->InitAsTranslationOpacity();

		m_pDC->CreateSolidColorBrush({}, &m_pBrush);
		const auto cx = GetWidth();
		const auto cy = GetHeight();
		m_LAVol.Create(L"100", Dui::DES_VISIBLE | Dui::DES_PARENT_COMP, 0,
			CxVolBarPadding, 0, CxVolLabel, cy, this);
		const auto x = CxVolBarPadding * 2 + CxVolLabel;
		m_TrackBar.Create(nullptr, Dui::DES_VISIBLE |
			Dui::DES_PARENT_COMP | Dui::DES_NOTIFY_TO_WND, 0,
			x, 0, cx - x - CxVolBarPadding, cy, this, nullptr, ELEID_VOLBAR_TRACK);
		m_TrackBar.SetRange(0, 200);
		m_TrackBar.SetTrackPos(100);
		m_TrackBar.SetTrackSize(CyVolTrack);
	}
	break;
	case WM_DESTROY:
		GetWnd()->UnregisterTimeLine(m_pecShowing);

		SafeRelease(m_pBrush);
		SafeRelease(m_pPageAn);
		SafeRelease(m_pecShowing);
		break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeVolumeBar::ShowAnimation()
{
	ECK_DUILOCK;
	ECKBOOLNOT(m_bShow);
	SetStyle(GetStyle() | Dui::DES_VISIBLE);
	SetCompositor(m_pPageAn);
	m_pecShowing->SetReverse(!m_bShow);
	m_pecShowing->Begin(eck::ECBF_CONTINUE);
	GetWnd()->WakeRenderThread();
}

void CVeVolumeBar::OnVolChanged(float fVol)
{
	WCHAR szVol[eck::CchI32ToStrBufNoRadix2];
	swprintf(szVol, L"%d", int(fVol));
	m_LAVol.SetText(szVol);
	m_LAVol.InvalidateRect();
}