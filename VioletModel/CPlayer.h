#pragma once
enum class PlayEvt
{
	Play,
	Pause,
	Resume,
	Stop,
	End,
	CommTick,
};

struct PLAY_EVT_PARAM
{
	PlayEvt eEvent;
};

enum class PlayErr
{
	Ok,
	NoPlayList,
	BassError,
	HResultError,
};

class CPlayList;
class CPlayer
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

	PlayErr PlayWorker(PLLITEM& e);

	void OnPlayEvent(const PLAY_EVT_PARAM& e);
public:
	CPlayer()
	{
		// 保证这是第一个槽
		m_Sig.Connect(this, &CPlayer::OnPlayEvent);
	}
	EckInlineNdCe auto& GetSignal() noexcept { return m_Sig; }
	EckInlineCe void SetList(CPlayList* pPlayList) noexcept { m_pPlayList = pPlayList; }
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
	PlayErr Stop();

	void SetPosition(double lfPos);
};