#include "pch.h"
#include "CWndMain.h"

void CPlayPanel::UpdateButtonImageSize()
{
	constexpr D2D1_SIZE_F Size{ CxyCircleButtonImage, CxyCircleButtonImage };
	m_BTPrev.SetImageSize(Size);
	m_BTPlay.SetImageSize(Size);
	m_BTNext.SetImageSize(Size);
	m_BTLrc.SetImageSize(Size);
	m_BTVol.SetImageSize(Size);
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

	case WM_NOTIFY:
	{
		const auto pElem = (Dui::CElem*)wParam;
		const auto pnm = (Dui::DUINMHDR*)lParam;
		switch (pnm->uCode)
		{
		case Dui::EE_CUSTOMDRAW:
		{
			if (pElem == &m_BTPrev ||pElem == &m_BTNext ||
				pElem == &m_BTVol ||pElem == &m_BTLrc)
			{
				const auto p = (Dui::CBTN_CUSTOM_DRAW*)lParam;
				if (p->dwStage == CDDS_PREPAINT)
				{
					if (p->eState == Dui::State::Hot ||
						p->eState == Dui::State::Selected)
					{
						const auto dxy = std::min(pElem->GetWidthF() / 2.f,
							pElem->GetHeightF() / 2.f);
						D2D1_ELLIPSE Ellipse{ { dxy, dxy }, dxy, dxy };
						if (p->eState == Dui::State::Hot)
							m_pBrush->SetColor(App->GetColor(GPal::CkBtnHot));
						else
							m_pBrush->SetColor(App->GetColor(GPal::CkBtnPushed));
						m_pDC->FillEllipse(Ellipse, m_pBrush);
					}

					m_pDC->DrawBitmap(p->pImg, p->rcImg, 1.f,
						((Dui::CCircleButton*)pElem)->GetInterpolationMode());
					return CDRF_SKIPDEFAULT;
				}
			}
		}
		break;
		case Dui::EE_COMMAND:
		{
			if (pElem == &m_BTPlay)
			{
				auto& Player = App->GetPlayer();
				if (Player.IsActive())
					Player.PlayOrPause();
				else
				{
					// TMPTMPTMP
					Player.Play(0);
				}
			}
		}
		break;
		}
	}
	break;

	case WM_SIZE:
	{
		// 移动右侧按钮
		const auto y = (GetHeight() - CxyCircleButton) / 2;
		int x = GetWidth() - CxyCircleButton - CxPaddingCircleButtonRightEdge;
		m_BTVol.SetPos(x, y);
		x -= (CxyCircleButton + CxPaddingCircleButton);

		m_BTLrc.SetPos(x, y);
		x -= (CxyCircleButton + CxPaddingCircleButton);

		constexpr int xLeftLimit = DLeftMiniCover + CxyMiniCover +
			CxPaddingPlayPanelText + CxMaxTitleAndArtist + CxPaddingPlayPanelText +
			CxMaxTime + CxPaddingPlayPanelText;

		x = xLeftLimit + ((x - xLeftLimit) - (CxyCircleButton * 2 + CxyCircleButtonBig +
			CxPaddingCircleButton * 2)) / 2;
		// 移动中间按钮
		m_BTPrev.SetPos(x, y);
		x += (CxyCircleButton + CxPaddingCircleButton);

		m_BTPlay.SetPos(x, (GetHeight() - CxyCircleButtonBig) / 2);
		x += (CxyCircleButtonBig + CxPaddingCircleButton);

		m_BTNext.SetPos(x, y);
		// 移动水印
		m_LAWatermark.SetRect({ GetWidth() / 2,0,GetWidth() - 10,GetHeight() - 10 });
	}
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
		m_Cover.Create(nullptr, Dui::DES_VISIBLE, 0,
			x, DTopMiniCover, CxyMiniCover, CxyMiniCover, this, pWnd);
		x += (CxyMiniCover + CxPaddingPlayPanelText);

		int y = DTopTitle;
		m_LATitle.Create(L"Violet", Dui::DES_VISIBLE, 0,
			x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
		y += (CyPlayPanelText + CyPaddingTitleAndArtist);
		m_LATitle.SetTextFormat(pWnd->TfGetLeft());

		m_LAArtist.Create(L"Player", Dui::DES_VISIBLE, 0,
			x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
		x += (CxMaxTitleAndArtist + CxPaddingPlayPanelText);
		m_LAArtist.SetTextFormat(pWnd->TfGetLeft());

		m_LATime.Create(L"00:00/00:00", Dui::DES_VISIBLE, 0,
			x, DTopTime, CxMaxTime, CyPlayPanelText, this, pWnd);
		m_LATime.SetTextFormat(pWnd->TfGetLeft());

		m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyCircleButton, CxyCircleButton, this, pWnd);
		m_BTPrev.SetImage(pWnd->RealizeImg(GImg::Prev));
		m_BTPrev.SetCustomDraw(TRUE);

		m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyCircleButtonBig, CxyCircleButtonBig, this, pWnd);
		m_BTPlay.SetImage(pWnd->RealizeImg(GImg::Triangle));
		m_BTPlay.SetCustomDraw(TRUE);

		m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyCircleButton, CxyCircleButton, this, pWnd);
		m_BTNext.SetImage(pWnd->RealizeImg(GImg::Next));
		m_BTNext.SetCustomDraw(TRUE);

		m_BTLrc.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyCircleButton, CxyCircleButton, this, pWnd);
		m_BTLrc.SetImage(pWnd->RealizeImg(GImg::Lrc));
		m_BTLrc.SetCustomDraw(TRUE);

		m_BTVol.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyCircleButton, CxyCircleButton, this, pWnd);
		m_BTVol.SetImage(pWnd->RealizeImg(GImg::PlayerVolume3));
		m_BTVol.SetCustomDraw(TRUE);

		UpdateButtonImageSize();
	}
	break;

	case WM_DESTROY:
		SafeRelease(m_pBrush);
		break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}
