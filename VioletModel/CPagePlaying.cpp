#include "pch.h"
#include "CPagePlaying.h"
#include "CApp.h"


void CPagePlaying::UpdateBlurredCover()
{
	ComPtr<IWICBitmap> pWicCover;
	App->GetPlayer().GetCover(pWicCover.RefOf());
	if (!pWicCover.Get())
		return;
	ECK_DUILOCK;

	ComPtr<ID2D1Image> pOldTarget;
	m_pDC->GetTarget(&pOldTarget);
	m_pDC->SetTarget(m_pBmpBlurredCover);
	m_pDC->SetTransform(D2D1::Matrix3x2F::Identity());
	m_pDC->BeginDraw();
	m_pDC->Clear(D2D1::ColorF(D2D1::ColorF::White));// TODO:主题色

	const auto cxElem = Log2PhyF(GetWidthF());
	const auto cyElem = Log2PhyF(GetHeightF());
	UINT cx0, cy0;	// 原始大小
	float cyRgn;	// 截取区域高
	float cx;		// 缩放后图片宽
	float cy;		// 缩放后图片高
	D2D_POINT_2F pt;// 画出位置
	// 情况一，客户区宽为最大边    cxClient / cxPic = cyClient / cyRgn
	// 情况二，客户区高为最大边    cyClient / cyPic = cxClient / cxRgn
	pWicCover->GetSize(&cx0, &cy0);
	cyRgn = cyElem / cxElem * (float)cx0;
	if (cyRgn < cy0)// 情况一
	{
		cx = cxElem;
		cy = cx * cy0 / cx0;
		pt = { 0.f,(cyElem - cy) / 2 };
	}
	else// 情况二
	{
		cy = (float)cyElem;
		cx = cx0 * cy / cy0;
		pt = { (cxElem - cx) / 2,0.f };
	}
	//---缩放
	ComPtr<IWICBitmap> pWicBmpScaled;
	eck::ScaleWicBitmap(pWicCover.Get(), pWicBmpScaled.RefOf(), (int)cx, (int)cy,
		WICBitmapInterpolationModeNearestNeighbor);
	SafeRelease(m_pBmpCover);
	m_pDC->CreateBitmapFromWicBitmap(pWicBmpScaled.Get(), &m_pBmpCover);
	m_Cover.SetBitmap(m_pBmpCover);
	//---模糊 
	ComPtr<ID2D1Effect> pEffect;
	m_pDC->CreateEffect(CLSID_D2D1GaussianBlur, &pEffect);
	pEffect->SetInput(0, m_pBmpCover);
	pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 40.f);
	pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
	pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION,
		D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);
	GetWnd()->Phy2Log(pt);
	m_pDC->DrawImage(pEffect.Get(), pt);
	//---半透明遮罩
	m_pDC->FillRectangle(GetViewRectF(), m_pBrBkg);

	m_pDC->EndDraw();
	m_pDC->SetTarget(pOldTarget.Get());
}

void CPagePlaying::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
	{
		m_Lrc.LrcTick(App->GetPlayer().GetCurrLrcIdx());
	}
	break;
	case PlayEvt::Play:
	{
		UpdateBlurredCover();
		InvalidateRect();

		m_Lrc.LrcInit(App->GetPlayer().GetLrc());
	}
	break;
	case PlayEvt::Stop:
	{
		m_Lrc.LrcClear();
	}
	break;
	}
}

LRESULT CPagePlaying::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		m_pDC->DrawBitmap(m_pBmpBlurredCover, ps.rcfClipInElem,
			1.f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, ps.rcfClipInElem);
		EndPaint(ps);
	}
	return 0;

	case WM_SIZE:
	{
		const auto cx = GetWidthF();
		const auto cy = GetHeightF();
		if (m_pBmpBlurredCover)
		{
			const auto size = m_pBmpBlurredCover->GetSize();
			if (!(size.width < cx || size.width / 2.f > cx ||
				size.height < cy || size.height / 2.f > cy))
				goto Update;
			SafeRelease(m_pBmpBlurredCover);
			GetWnd()->BmpNewLogSize(cx, cy, m_pBmpBlurredCover);
		}
		else
			GetWnd()->BmpNewLogSize(cx, cy, m_pBmpBlurredCover);
	Update:;
		UpdateBlurredCover();

		m_Lrc.SetRect({
			300, 50, GetWidth() - 20, GetHeight() - 150});
	}
	break;
	case WM_DPICHANGED:
	{
		const auto cx = GetWidthF();
		const auto cy = GetHeightF();
		SafeRelease(m_pBmpBlurredCover);
		GetWnd()->BmpNewLogSize(cx, cy, m_pBmpBlurredCover);
		UpdateBlurredCover();
	}
	break;

	case WM_CREATE:
	{
		App->GetPlayer().GetSignal().Connect(this, &CPagePlaying::OnPlayEvent);
		
		m_Cover.Create(nullptr, Dui::DES_VISIBLE, 0,
			50, 50, 200, 200, this);
		m_Lrc.Create(nullptr, Dui::DES_VISIBLE, 0,
			50, 260, 200, 100, this);

		D2D1_COLOR_F crLrc[CVeLrc::CriMax]
		{
			{.a = 0.6f },
			{.a = 1.f },
		};
		m_Lrc.SetColor(crLrc);

		ComPtr<IDWriteTextFormat> pTfLrc;
		auto& FontFactory = App->GetFontFactory();;
		FontFactory.NewFont(pTfLrc.RefOfClear(),
			eck::Align::Near, eck::Align::Near, 20, 500);
		m_Lrc.SetTextFormat(pTfLrc.Get());
		FontFactory.NewFont(pTfLrc.RefOfClear(),
			eck::Align::Near, eck::Align::Near, 18, 500);
		m_Lrc.SetTextFormatTrans(pTfLrc.Get());

		m_BTBack.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_TO_WND, 0,
			100, 100, 70, 20, this);
		m_BTBack.SetID(ELEID_PLAYPAGE_BACK);

		m_pDC->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &m_pBrBkg);
	}
	break;
	case WM_DESTROY:
	{
		SafeRelease(m_pBmpBlurredCover);
		SafeRelease(m_pBmpCover);
		SafeRelease(m_pBrBkg);
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}