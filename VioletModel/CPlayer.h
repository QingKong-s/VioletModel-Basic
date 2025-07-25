#pragma once
#include "CPlayList.h"

enum class PlayEvt
{
	Play,
	Pause,
	Resume,
	Stop,
	End,
	CommTick,
	ListChanged,
};

struct PLAY_EVT_PARAM
{
	PlayEvt eEvent;
};

enum class PlayErr
{
	Ok,
	NoPlayList,		// 没有播放列表
	ErrBass,		// Bass报告了错误
	ErrHResult,		// HRESULT错误
	ErrUnexpectedPlayingState,// CPlayer::PlayOrPause使用，Bass播放状态异常
	ErrNoCurrItem,	// 当前未播放任何项
};

class CPlayer final
{
private:
	// 该信号上的所有操作都在UI线程执行
	eck::CSignal<eck::NoIntercept_T, void, const PLAY_EVT_PARAM&> m_Sig{};
	CBass m_Bass{};

	IWICBitmap* m_pBmpCover{};
	CPlayList* m_pPlayList{};

	Tag::MUSICINFO m_MusicInfo{};

	double m_lfCurrTime{};// 秒
	double m_lfTotalTime{};// 秒
	BITBOOL m_bActive : 1{};

	PlayErr PlayWorker(CPlayList::ITEM& e);

	void OnPlayEvent(const PLAY_EVT_PARAM& e);
public:
	CPlayer()
	{
		// 保证这是第一个槽
		m_Sig.Connect(this, &CPlayer::OnPlayEvent);
	}

	~CPlayer();

	EckInlineNdCe auto& GetSignal() noexcept { return m_Sig; }
	EckInline void SetList(CPlayList* pPlayList) noexcept
	{
		if (m_pPlayList == pPlayList)
			return;
		m_pPlayList = pPlayList;
		GetSignal().Emit(PLAY_EVT_PARAM{ PlayEvt::ListChanged });
	}
	EckInlineNdCe CPlayList* GetList() const noexcept { return m_pPlayList; }
	EckInlineNdCe BOOL IsActive() const noexcept { return m_bActive; }
	// 秒
	EckInlineNdCe double GetCurrTime() const noexcept { return m_lfCurrTime; }
	// 秒
	EckInlineNdCe double GetTotalTime() const noexcept { return m_lfTotalTime; }
	EckInlineNdCe auto& GetBass() noexcept { return m_Bass; }

	PlayErr Play(int idx);
	PlayErr Play(int idxGroup, int idxItem);
	PlayErr PlayOrPause();
	PlayErr Stop(BOOL bNoGap = FALSE);
	PlayErr Next();
	PlayErr Prev();
	PlayErr AutoNext();

	// 秒
	void SetPosition(double lfPos);

	// 返回后调用方持有一份引用
	void GetCover(_Out_ IWICBitmap*& pBmp)
	{
		if (m_pBmpCover)
			m_pBmpCover->AddRef();
		pBmp = m_pBmpCover;
	}

	EckInlineNdCe auto& GetMusicInfo() const noexcept { return m_MusicInfo; }
};