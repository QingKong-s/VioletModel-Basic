#include "pch.h"
#include "CApp.h"
#include "CPlayList.h"

void CPlayer::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
	{
		m_lfCurrTime = m_Bass.GetPosition();
		LrcUpdatePosition();
	}
	break;
	}
}

CPlayer::~CPlayer()
{
	SafeRelease(m_pBmpCover);
}

PlayErr CPlayer::PlayWorker(CPlayList::ITEM& e)
{
	m_bActive = TRUE;
	m_bPaused = FALSE;
	if (!m_Bass.Open(e.rsFile.Data()))
	{
		m_dwLastHrOrBassErr = CBass::GetError();
		m_bActive = FALSE;
		return PlayErr::ErrBass;
	}
	m_Bass.TempoCreate();
	m_Bass.Play(TRUE);
	m_lfCurrTime = 0;
	m_lfTotalTime = m_Bass.GetLength();
	m_Bass.SetSync(BASS_SYNC_END | BASS_SYNC_ONETIME, 0,
		[](DWORD, DWORD, DWORD, PVOID pUser)
		{
			((eck::THREADCTX*)pUser)->Callback.EnQueueCallback([]
				{
					App->GetPlayer().m_Sig.Emit({ PlayEvt::End });
				});
		}, eck::GetThreadCtx());

	Tag::CMediaFile mf{ e.rsFile.Data(),STGM_READ };
	const auto uTag = mf.DetectTag();
	m_MusicInfo.uFlag = Tag::MIF_JOIN_ARTIST;
	m_MusicInfo.uMask = Tag::MIM_ALL;
	if (uTag & (Tag::TAG_ID3V2_3 | Tag::TAG_ID3V2_4))
	{
		Tag::CID3v2 Id3v2{ mf };
		Id3v2.ReadTag(0);
		Id3v2.SimpleExtract(m_MusicInfo);
	}
	else if (uTag & Tag::TAG_FLAC)
	{
		Tag::CFlac Flac{ mf };
		Flac.ReadTag(0);
		Flac.SimpleExtract(m_MusicInfo);
	}
	else if (uTag & Tag::TAG_APE)
	{
		Tag::CApe Ape{ mf };
		Ape.ReadTag(0);
		Ape.SimpleExtract(m_MusicInfo);
	}
	else if (uTag & Tag::TAG_ID3V1)
	{
		Tag::CID3v1 Id3v1{ mf };
		Id3v1.ReadTag(0);
		Id3v1.SimpleExtract(m_MusicInfo);
	}
	const auto pPic = m_MusicInfo.GetMainCover();
	SafeRelease(m_pBmpCover);
	if (pPic)
	{
		if (pPic->bLink)
			m_dwLastHrOrBassErr = eck::CreateWicBitmap(
				m_pBmpCover, std::get<1>(pPic->varPic).Data());
		else
		{
			const auto pStream = new eck::CStreamView{ std::get<0>(pPic->varPic) };
			m_dwLastHrOrBassErr = eck::CreateWicBitmap(m_pBmpCover, pStream);
			pStream->Release();
		}
		if (FAILED(m_dwLastHrOrBassErr))
			return PlayErr::ErrHResult;
	}
	else
	{
		m_pBmpCover = App->GetImg(GImg::DefaultCover);
		m_pBmpCover->AddRef();
	}

	m_pvLrc = std::make_shared<std::vector<eck::LRCINFO>>();
	m_pvLrcLabel = std::make_shared<std::vector<eck::LRCLABEL>>();

	auto rsLrcPath{ e.rsFile };
	rsLrcPath.PazRenameExtension(EckStrAndLen(L".lrc"));
	eck::ParseLrc(rsLrcPath.Data(), 0u, *m_pvLrc, *m_pvLrcLabel,
		eck::LrcEncoding::Auto, (float)m_lfTotalTime);
	if (!m_pvLrc->size())
	{
		eck::ParseLrc(m_MusicInfo.rsLrc.Data(), m_MusicInfo.rsLrc.ByteSize(),
			*m_pvLrc, *m_pvLrcLabel,
			eck::LrcEncoding::UTF16LE, (float)m_Bass.GetLength());
	}

	GetSignal().Emit({ PlayEvt::Play });
	return PlayErr::Ok;
}

PlayErr CPlayer::Play(int idx)
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	Stop(TRUE);
	GetList()->PlySetCurrentItem(idx);
	return PlayWorker(GetList()->FlAt(idx));
}

PlayErr CPlayer::Play(int idxGroup, int idxItem)
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	Stop(TRUE);
	GetList()->PlySetCurrentItem(idxGroup, idxItem);
	return PlayWorker(GetList()->GrAt(idxGroup, idxItem));
}

PlayErr CPlayer::PlayOrPause()
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	if (m_bActive)
	{
		switch (m_Bass.IsActive())
		{
		case BASS_ACTIVE_PLAYING:
			m_Bass.Pause();
			m_bPaused = TRUE;
			GetSignal().Emit({ PlayEvt::Pause });
			return PlayErr::Ok;
		case BASS_ACTIVE_PAUSED:
			m_Bass.Play();
			m_bPaused = FALSE;
			GetSignal().Emit({ PlayEvt::Resume });
			return PlayErr::Ok;
		}
		return PlayErr::UnexpectedPlayingState;
	}
	else
	{
		if (GetList()->FlGetCount())
			Play(0);
	}
	return PlayErr::Ok;
}

PlayErr CPlayer::Stop(BOOL bNoGap)
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	m_Bass.Stop();
	m_Bass.Close();
	m_idxCurrLrc = m_idxLastLrc = -1;
	if (!bNoGap)
	{
		m_bActive = FALSE;
		m_Sig.Emit({ PlayEvt::Stop });
		if (GetList()->IsGroupEnabled())
			GetList()->PlySetCurrentItem(-1, -1);
		else
			GetList()->PlySetCurrentItem(-1);
	}
	return PlayErr::Ok;
}

PlayErr CPlayer::Next()
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	int idxItem, idxGroup;
	if (GetList()->IsGroupEnabled())
	{
		idxItem = GetList()->PlyGetCurrentItem(idxGroup);
		if (idxItem < 0)
			return PlayErr::NoPlayList;
		const auto cCurrGroupItem = (int)GetList()->GrAtGroup(idxGroup).vItem.size();
		++idxItem;
		if (idxItem >= cCurrGroupItem)
		{
			idxItem = 0;
			++idxGroup;
			if (idxGroup >= GetList()->GrGetGroupCount())
				idxGroup = 0;// 回到第一个组
		}
		return Play(idxGroup, idxItem);
	}
	else
	{
		idxItem = GetList()->PlyGetCurrentItem();
		if (idxItem < 0)
			return PlayErr::NoPlayList;
		++idxItem;
		if (idxItem >= GetList()->FlGetCount())
			idxItem = 0;
		return Play(idxItem);
	}
}

PlayErr CPlayer::Prev()
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	int idxItem, idxGroup;
	if (GetList()->IsGroupEnabled())
	{
		idxItem = GetList()->PlyGetCurrentItem(idxGroup);
		if (idxItem < 0)
			return PlayErr::NoPlayList;
		if (idxItem <= 0)
		{
			idxGroup--;
			if (idxGroup < 0)
				idxGroup = GetList()->GrGetGroupCount() - 1;// 回到最后一个组
			idxItem = (int)GetList()->GrAtGroup(idxGroup).vItem.size() - 1;
		}
		else
			--idxItem;
		return Play(idxGroup, idxItem);
	}
	else
	{
		idxItem = GetList()->PlyGetCurrentItem();
		if (idxItem < 0)
			return PlayErr::NoPlayList;
		if (idxItem <= 0)
			idxItem = GetList()->FlGetCount() - 1;
		else
			--idxItem;
		return Play(idxItem);
	}
}

PlayErr CPlayer::AutoNext()
{
	return PlayErr();
}

void CPlayer::SetPosition(double lfPos)
{
	m_Bass.SetPosition(lfPos);
	m_lfCurrTime = m_Bass.GetPosition();
}

BOOL CPlayer::LrcUpdatePosition()
{
	if (!m_pvLrc || m_pvLrc->empty())
		return FALSE;
	const auto fPos = (float)m_lfCurrTime;
	const auto& vLrc = *m_pvLrc;
	const auto cLrc = (int)vLrc.size();
	if (m_idxCurrLrc >= 0)
	{
		if (m_idxCurrLrc + 1 < cLrc)
		{
			if (fPos >= vLrc[m_idxCurrLrc].fTime &&
				fPos < vLrc[m_idxCurrLrc + 1].fTime)
				return FALSE;
			else if (m_idxCurrLrc + 2 < cLrc &&
				fPos >= vLrc[m_idxCurrLrc + 1].fTime &&
				fPos < vLrc[m_idxCurrLrc + 2].fTime)
			{
				++m_idxCurrLrc;
				return TRUE;
			}
		}
		else if (fPos >= vLrc[m_idxCurrLrc].fTime)
			return FALSE;
	}
	const auto it = std::lower_bound(vLrc.begin(), vLrc.end(), fPos,
		[](const eck::LRCINFO& Item, float fPos)->bool
		{
			return Item.fTime < fPos;
		});
	if (it == vLrc.end())
		m_idxCurrLrc = (int)vLrc.size() - 1;
	else if (it == vLrc.begin())
		m_idxCurrLrc = -1;
	else
		m_idxCurrLrc = (int)std::distance(vLrc.begin(), it - 1);
	EckAssert(m_idxCurrLrc >= -1 && m_idxCurrLrc < (int)vLrc.size());

	if (m_idxCurrLrc != m_idxLastLrc)
	{
		m_idxLastLrc = m_idxCurrLrc;
		return TRUE;
	}
	return FALSE;
}