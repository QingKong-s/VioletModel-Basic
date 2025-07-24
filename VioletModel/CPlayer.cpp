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
	if (!m_Bass.Open(e.rsFile.Data()))
		return PlayErr::ErrBass;
	m_Bass.TempoCreate();
	m_Bass.Play(TRUE);
	m_lfCurrTime = 0;
	m_lfTotalTime = m_Bass.GetLength();
	m_Bass.SetSync(BASS_SYNC_END | BASS_SYNC_ONETIME, 0,
		[](DWORD, DWORD, DWORD, PVOID pUser)
		{
			((eck::THREADCTX*)pUser)->Callback.EnQueueCallback([]
				{
					PLAY_EVT_PARAM pep;
					pep.eEvent = PlayEvt::End;
					App->GetPlayer().m_Sig.Emit(pep);
				});
		}, eck::GetThreadCtx());

	Tag::CMediaFile mf{ e.rsFile.Data(),STGM_READ };
	mf.DetectTag();
	Tag::CID3v2 Id3v2{ mf };
	Id3v2.ReadTag(0);
	m_MusicInfo.uFlag = Tag::MIF_JOIN_ARTIST;
	m_MusicInfo.uMask = Tag::MIM_ALL;
	Id3v2.SimpleExtract(m_MusicInfo);
	const auto pPic = m_MusicInfo.GetMainCover();
	SafeRelease(m_pBmpCover);
	if (pPic)
	{
		HRESULT hr;
		if (pPic->bLink)
			hr = eck::CreateWicBitmap(m_pBmpCover, std::get<1>(pPic->varPic).Data());
		else
		{
			auto pStream = new eck::CStreamView(std::get<0>(pPic->varPic));
			hr = eck::CreateWicBitmap(m_pBmpCover, pStream);
			pStream->Release();
		}
		if (FAILED(hr))
			return PlayErr::ErrHResult;
	}
	else
	{
		m_pBmpCover = App->GetImg(GImg::DefaultCover);
		m_pBmpCover->AddRef();
	}
	GetSignal().Emit(PLAY_EVT_PARAM{ PlayEvt::Play });
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
			GetSignal().Emit(PLAY_EVT_PARAM{ PlayEvt::Pause });
			return PlayErr::Ok;
		case BASS_ACTIVE_PAUSED:
			m_Bass.Play();
			GetSignal().Emit(PLAY_EVT_PARAM{ PlayEvt::Resume });
			return PlayErr::Ok;
		}
		return PlayErr::ErrUnexpectedPlayingState;
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
	if (!bNoGap)
	{
		m_bActive = FALSE;
		m_Sig.Emit(PLAY_EVT_PARAM{ PlayEvt::Stop });
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
		Play(idxGroup, idxItem);
	}
	else
	{
		idxItem = GetList()->PlyGetCurrentItem();
		if (idxItem < 0)
			return PlayErr::NoPlayList;
		++idxItem;
		if (idxItem >= GetList()->FlGetCount())
			idxItem = 0;
		Play(idxItem);
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
