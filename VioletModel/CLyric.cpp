#include "pch.h"
#include "CLyric.h"

HRESULT CLyric::LrcGetLyric(int idx, _Out_ LYRIC_LINE& ll)
{
	if (idx < 0 || idx >= LrcGetCount())
		return E_BOUNDS;
	ll.pszLrc = m_vLrc[idx].pszLrc;
	ll.pszTranslation = m_vLrc[idx].pszTranslation;
	ll.cchLrc = m_vLrc[idx].cchLrc;
	ll.cchTranslation = m_vLrc[idx].cchTotal - m_vLrc[idx].cchLrc;
	ll.fTime = m_vLrc[idx].fTime;
	ll.fDuration = m_vLrc[idx].fDuration;
	return S_OK;
}

HRESULT CLyric::LrcGetCurrentTimeInfo(int idx, _Out_ LYRIC_CURR_TIME& lct)
{
	if (idx < 0 || idx >= LrcGetCount())
		return E_BOUNDS;
	lct.fCurrTime = m_fCurrTime;
	lct.fLrcTime = m_vLrc[idx].fTime;
	lct.fLrcDuration = m_vLrc[idx].fDuration;
	return S_OK;
}