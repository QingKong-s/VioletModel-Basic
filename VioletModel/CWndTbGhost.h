#pragma once
class CWndTbGhost final :public eck::CWnd
{
	friend class CWndMain;
private:
	CWndMain& m_WndMain;
	HBITMAP m_hbmLivePreviewCache{};// 实时预览位图缓存，保留
	HBITMAP m_hbmThumbnailCache{};	// 缩略图位图缓存

	UINT m_cxPrev = 0u, m_cyPrev = 0u;
public:
	CWndTbGhost(CWndMain& w) :m_WndMain{ w } {}
	ECK_CWND_CREATE_CLS_HINST(eck::WCN_DUMMY, eck::g_hInstance);

	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// 保留
	void InvalidateLivePreviewCache()
	{
		if (m_hbmLivePreviewCache)
		{
			DeleteObject(m_hbmLivePreviewCache);
			m_hbmLivePreviewCache = nullptr;
		}
	}

	void InvalidateThumbnailCache()
	{
		if (m_hbmThumbnailCache)
		{
			DeleteObject(m_hbmThumbnailCache);
			m_hbmThumbnailCache = nullptr;
		}
	}

	void SetIconicThumbnail(UINT cxMax = UINT_MAX, UINT cyMax = UINT_MAX);
};