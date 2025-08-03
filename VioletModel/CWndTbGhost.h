#pragma once
class CWndTbGhost final :public eck::CWnd
{
	friend class CWndMain;
private:
	CWndMain& m_WndMain;
	HBITMAP m_hbmLivePreviewCache{};// 实时预览位图缓存
	HBITMAP m_hbmThumbnailCache{};	// 缩略图位图缓存

	UINT m_cxPrev{}, m_cyPrev{};
public:
	CWndTbGhost(CWndMain& w) :m_WndMain{ w } {}
	ECK_CWND_CREATE_CLS_HINST(eck::WCN_DUMMY, eck::g_hInstance);

	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// 保留，当前禁止调用
	void InvalidateLivePreviewCache();

	void InvalidateThumbnailCache();
	EckInline HRESULT InvalidateDwmThumbnail()
	{
		return DwmInvalidateIconicBitmaps(HWnd);
	}

	// 使用播放器当前封面更新任务栏缩略图，若尺寸设为UINT_MAX则使用上次记录的最适尺寸
	// 封面变化后必须立即更新一次
	void SetIconicThumbnail(UINT cxMax = UINT_MAX, UINT cyMax = UINT_MAX);
};