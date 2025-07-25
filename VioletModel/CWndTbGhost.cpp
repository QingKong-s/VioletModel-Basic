#include "pch.h"
#include "CWndMain.h"


LRESULT CWndTbGhost::OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		constexpr BOOL b = TRUE;
		DwmSetWindowAttribute(hWnd, DWMWA_HAS_ICONIC_BITMAP, &b, sizeof(b));
		DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &b, sizeof(b));
	}
	return 0;

	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
	{
		if (!m_hbmLivePreviewCache)
		{
			BITMAPINFO bi{};
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biWidth = bi.bmiHeader.biHeight = 1;
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biBitCount = 32;
			bi.bmiHeader.biCompression = BI_RGB;
			DWORD* pdwBits;
			m_hbmLivePreviewCache = CreateDIBSection(nullptr,
				&bi, DIB_RGB_COLORS, (void**)&pdwBits, nullptr, 0);
			*pdwBits = 0x01000000;
		}
		DwmSetIconicLivePreviewBitmap(hWnd, m_hbmLivePreviewCache, nullptr, 0);
	}
	return 0;

	case WM_DWMSENDICONICTHUMBNAIL:
		SetIconicThumbnail(HIWORD(lParam), LOWORD(lParam));
		return 0;

	case WM_ACTIVATE:
	{
		m_WndMain.m_pTaskbarList->SetTabActive(hWnd, m_WndMain.HWnd, 0);
		if (IsIconic(m_WndMain.HWnd))
			m_WndMain.Show(SW_RESTORE);
		SetForegroundWindow(m_WndMain.HWnd);
	}
	return 0;

	case WM_COMMAND:
	case WM_SYSCOMMAND:
		return m_WndMain.SendMsg(uMsg, wParam, lParam);

	case WM_DESTROY:
		m_WndMain.m_pTaskbarList->UnregisterTab(hWnd);
		InvalidateLivePreviewCache();
		InvalidateThumbnailCache();
		return 0;
	}
	return __super::OnMsg(hWnd, uMsg, wParam, lParam);
}

void CWndTbGhost::SetIconicThumbnail(UINT cxMax, UINT cyMax)
{
	if (cxMax == UINT_MAX || cyMax == UINT_MAX)
	{
		cxMax = (m_cxPrev ? m_cxPrev : eck::DpiScale(120, m_WndMain.GetDpiValue()));
		cyMax = (m_cyPrev ? m_cyPrev : eck::DpiScale(120, m_WndMain.GetDpiValue()));
	}
	m_cxPrev = cxMax;
	m_cyPrev = cyMax;

	if (m_hbmThumbnailCache)
	{
		BITMAP bmp;
		GetObjectW(m_hbmThumbnailCache, sizeof(bmp), &bmp);
		if (bmp.bmWidth <= (int)cxMax && bmp.bmHeight <= (int)cyMax)
		{
			DwmSetIconicThumbnail(HWnd, m_hbmThumbnailCache, 0);
			return;
		}
		else
			InvalidateThumbnailCache();
	}

	ComPtr<IWICBitmap> pOrg;
	App->GetPlayer().GetCover(pOrg.RefOf());
	if (!pOrg.Get())
		pOrg = App->GetImg(GImg::DefaultCover);
	UINT cx, cy, cx0, cy0;
	pOrg->GetSize(&cx0, &cy0);
	if ((float)cxMax / (float)cyMax > (float)cx0 / (float)cy0)// y对齐
	{
		cy = cyMax;
		cx = cx0 * cy / cy0;
	}
	else// x对齐
	{
		cx = cxMax;
		cy = cx * cy0 / cx0;
	}
	IWICBitmap* pBmp;
	eck::ScaleWicBitmap(pOrg.Get(), pBmp, cx, cy, WICBitmapInterpolationModeFant);
	m_hbmThumbnailCache = eck::CreateHBITMAP(pBmp);
	DwmSetIconicThumbnail(HWnd, m_hbmThumbnailCache, 0);
	pBmp->Release();
	return;
}