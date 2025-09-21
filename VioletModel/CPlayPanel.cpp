#include "pch.h"
#include "CWndMain.h"


void CPlayPanel::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
	{
		const auto& Player = App->GetPlayer();
		const auto lfCurrTime = Player.GetCurrTime();
		const auto lfTotalTime = Player.GetTotalTime();
		m_LATime.SetText(eck::Format(L"%02d:%02d/%02d:%02d",
			int(lfCurrTime / 60), int(lfCurrTime) % 60,
			int(lfTotalTime / 60), int(lfTotalTime) % 60).Data());
		m_LATime.InvalidateRect();
	}
	break;
	case PlayEvt::Play:
	{
		const auto& mi = App->GetPlayer().GetMusicInfo();
		m_LATitle.SetText(mi.rsTitle.Data());
		m_LAArtist.SetText(mi.slArtist.FrontData());
	}
	break;
	}
}

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

	case WM_SIZE:
		m_LAWatermark.SetRect({ GetWidthF() / 2.f,0,GetWidthF() - 10.f,GetHeightF() - 10.f });
		return 0;

	case WM_SETFONT:
	{
		m_LAArtist.SetTextFormat(GetTextFormat());
		m_LATime.SetTextFormat(GetTextFormat());
	}
	break;

	case Dui::EWM_COLORSCHEMECHANGED:
	{
		m_LAWatermark.SetColor(App->GetColor(GPal::PlayPanelWatermark));
		D2D1_COLOR_F cr;
		GetTheme()->GetSysColor(Dui::SysColor::Text, cr);
		m_LATitle.SetColor(cr);
		m_LATitle.UpdateFadeColor();
		m_LAArtist.SetColor(cr);
		m_LAArtist.UpdateFadeColor();
	}
	break;

	case WM_CREATE:
	{
		App->GetPlayer().GetSignal().Connect(this, &CPlayPanel::OnPlayEvent);
		m_pDC->CreateSolidColorBrush({}, &m_pBrush);
		const auto pWnd = (CWndMain*)GetWnd();

		m_LAWatermark.Create(L"VioletModel\r\n内部测试", Dui::DES_VISIBLE, 0,
			0, 0, 10, 10, this, pWnd);
		ComPtr<IDWriteTextFormat> pTfWatermark, pTfTitle;
		App->GetFontFactory().NewFont(pTfWatermark.RefOf(), eck::Align::Far,
			eck::Align::Far, (float)CyFontNormal);
		App->GetFontFactory().NewFont(pTfTitle.RefOf(), eck::Align::Near,
			eck::Align::Center, (float)CyFontNormal, 700);
		pTfTitle->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		m_LAWatermark.SetUserColor(TRUE);
		m_LAWatermark.SetColor(App->GetColor(GPal::PlayPanelWatermark));
		m_LAWatermark.SetTextFormat(pTfWatermark.Get());

		float x = DLeftMiniCover;
		m_Cover.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_TO_WND, 0,
			x, DTopMiniCover, CxyMiniCover, CxyMiniCover, this, pWnd);
		x += (CxyMiniCover + CxPaddingPlayPanelText);

		D2D1_COLOR_F crText;
		GetTheme()->GetSysColor(Dui::SysColor::Text, crText);
		float y = DTopTitle;
		m_LATitle.Create(L"Violet", Dui::DES_VISIBLE, 0,
			x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
		m_LATitle.SetColor(crText);
		m_LATitle.SetFade(TRUE);
		m_LATitle.SetTextFormat(pTfTitle.Get());
		y += (CyPlayPanelText + CyPaddingTitleAndArtist);

		m_LAArtist.Create(L"Player", Dui::DES_VISIBLE, 0,
			x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
		m_LAArtist.SetColor(crText);
		m_LAArtist.SetFade(TRUE);
		x += (CxMaxTitleAndArtist + CxPaddingPlayPanelText);

		m_LATime.Create(L"00:00/00:00", Dui::DES_VISIBLE, 0,
			x, DTopTime, CxMaxTime, CyPlayPanelText, this, pWnd);
	}
	break;

	case WM_DESTROY:
		SafeRelease(m_pBrush);
		break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}
