#pragma once
struct LYRIC_LINE
{
	PCWCH pszLrc;
	PCWCH pszTranslation;
	int cchLrc;
	int cchTranslation;
	float fTime;
	float fDuration;
};

struct LYRIC_CURR_TIME
{
	float fCurrTime;	// 播放器当前时间
	float fLrcTime;		// 当前行开始时间
	float fLrcDuration;	// 当前行持续时间
};

class CLyric : public eck::CRefObj<CLyric>
{
	friend class CPlayer;
private:
	std::vector<eck::LRCINFO> m_vLrc{};
	std::vector<eck::LRCLABEL> m_vLabel{};
	float m_fCurrTime{};

	EckInlineNdCe auto& LrcpGetLrc() { return m_vLrc; }
	EckInlineNdCe auto& LrcpGetLabel() { return m_vLabel; }
	EckInlineCe void LrcpSetCurrentTime(float fTime) { m_fCurrTime = fTime; }
public:
	EckInlineNdCe int LrcGetCount() const { return (int)m_vLrc.size(); }
	EckInlineNdCe float LrcGetCurrentTime() const { return m_fCurrTime; }
	HRESULT LrcGetLyric(int idx, _Out_ LYRIC_LINE& ll);
	HRESULT LrcGetCurrentTimeInfo(int idx, _Out_ LYRIC_CURR_TIME& lct);
};