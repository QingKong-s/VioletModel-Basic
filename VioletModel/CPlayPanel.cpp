#include "pch.h"
#include "CWndMain.h"


LRESULT CPlayPanel::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		m_pBrush->SetColor(App->GetColor(GPal::PlayPanelBk));
		m_pDC->FillRectangle(ps.rcfClipInElem, m_pBrush);
		EndPaint(ps);
	}
	return 0;

	case WM_NOTIFY:
	{

	}
	break;

	case WM_SIZE:
		m_LAWatermark.SetRect({ GetWidth() / 2,0,GetWidth() - 10,GetHeight() - 10 });
		return 0;

	case WM_CREATE:
	{
		m_pDC->CreateSolidColorBrush({}, &m_pBrush);
		const auto pWnd = (CWndMain*)GetWnd();

		constexpr static WCHAR Watermark[]
		{
			L"VioletModel\n"
			L"内部测试"
		};
		m_LAWatermark.Create(Watermark, Dui::DES_VISIBLE, 0,
			0, 0, 10, 10, this, pWnd);
		auto pTfWatermark = pWnd->TfClone();
		pTfWatermark->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		pTfWatermark->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		m_LAWatermark.SetTextFormat(pTfWatermark);
		pTfWatermark->Release();
		m_LAWatermark.SetUseThemeColor(FALSE);
		m_LAWatermark.SetTextColor(App->GetColor(GPal::PlayPanelWatermark));

		int x = DLeftMiniCover;
		m_Cover.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_TO_WND, 0,
			x, DTopMiniCover, CxyMiniCover, CxyMiniCover, this, pWnd);
		x += (CxyMiniCover + CxPaddingPlayPanelText);

		int y = DTopTitle;
		IDWriteTextFormat* pTfBold;
		LOGFONTW lf;
		eck::GetDefFontInfo(lf);
		eck::g_pDwFactory->CreateTextFormat(
			lf.lfFaceName,
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			lf.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			15.f,
			L"zh-cn",
			&pTfBold);
		m_LATitle.Create(L"Violet", Dui::DES_VISIBLE, 0,
			x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
		y += (CyPlayPanelText + CyPaddingTitleAndArtist);
		m_LATitle.SetTextFormat(pTfBold);
		pTfBold->Release();

		m_LAArtist.Create(L"Player", Dui::DES_VISIBLE, 0,
			x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
		x += (CxMaxTitleAndArtist + CxPaddingPlayPanelText);
		m_LAArtist.SetTextFormat(pWnd->TfGetLeft());

		m_LATime.Create(L"00:00/00:00", Dui::DES_VISIBLE, 0,
			x, DTopTime, CxMaxTime, CyPlayPanelText, this, pWnd);
		m_LATime.SetTextFormat(pWnd->TfGetLeft());
	}
	break;

	case WM_DESTROY:
		SafeRelease(m_pBrush);
		break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}
