#include "pch.h"
#include "CApp.h"
#include "CPlayList.h"
#include "Utils.h"

void CPlayer::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
		m_lfCurrTime = m_Bass.GetPosition();
		LrcUpdatePosition();
		break;
	case PlayEvt::End:
		AutoNext();
		break;
	}
}

CPlayer::~CPlayer()
{
	SafeRelease(m_pBmpCover);
	SafeRelease(m_pLrc);
}

void CPlayer::SetList(CPlayList* pPlayList) noexcept
{
	if (m_pPlayList == pPlayList)
		return;
	m_pPlayList = pPlayList;
	GetSignal().Emit({ PlayEvt::ListChanged });
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
	m_MusicInfo.uMask = Tag::MIM_ALL;
	Tag::SIMPLE_OPT Opt{};
	Opt.svArtistDiv = {};
	Opt.svCommDiv = {};
	VltGetMusicInfo(e.rsFile.Data(), m_MusicInfo, Opt);
	const auto pPic = m_MusicInfo.GetMainCover();
	SafeRelease(m_pBmpCover);
	if (pPic)
	{
		m_bDefCover = FALSE;
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
			goto UseDefCover;
	}
	else
	{
	UseDefCover:
		m_bDefCover = TRUE;
		m_pBmpCover = App->GetImg(GImg::DefaultCover);
		m_pBmpCover->AddRef();
	}

	SafeRelease(m_pLrc);
	m_pLrc = new Lyric::CLyric{};

	auto rsLrcPath{ e.rsFile };
	rsLrcPath.PazRenameExtension(EckStrAndLen(L".lrc"));
	m_pLrc->MgSetDuration((float)m_lfTotalTime);
	if (NT_SUCCESS(m_pLrc->LoadTextFile(rsLrcPath.Data())) &&
		!m_pLrc->IsTextEmpty())
		m_pLrc->ParseLrc();
	if (!m_pLrc->MgGetLineCount() && !m_MusicInfo.rsLrc.IsEmpty())
	{
		m_pLrc->LoadTextMove(std::move(m_MusicInfo.rsLrc));
		m_pLrc->ParseLrc();
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
	return PlayWorker(GetList()->FlAtAbs(idx));
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
		m_bDefCover = TRUE;
		m_bActive = FALSE;
		m_Sig.Emit({ PlayEvt::Stop });
		if (GetList()->IsGroupEnabled())
			GetList()->PlySetCurrentItem(-1, -1);
		else
			GetList()->PlySetCurrentItem(-1);
	}
	return PlayErr::Ok;
}

PlayErr CPlayer::Next(BOOL bNoLoop)
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
			if (bNoLoop)
				return PlayErr::ListEnd;
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
		{
			if (bNoLoop)
				return PlayErr::ListEnd;
			idxItem = 0;
		}
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
	switch (m_eAutoNextMode)
	{
	case AutoNextMode::ListLoop:
		return Next();
	case AutoNextMode::List:
	{
		const auto r = Next(TRUE);
		if (r == PlayErr::ListEnd)
			return Stop();
		return r;
	}
	case AutoNextMode::Radom:
		break;// TODO 随机播放
	case AutoNextMode::SingleLoop:
		m_Bass.Play(TRUE);
		return PlayErr::Ok;
	case AutoNextMode::Single:
		return Stop();
	}
	return PlayErr::Ok;
}

void CPlayer::SetPosition(double lfPos)
{
	m_Bass.SetPosition(lfPos);
	m_lfCurrTime = m_Bass.GetPosition();
}

AutoNextMode CPlayer::NextAutoNextMode()
{
	if (++m_eAutoNextMode >= AutoNextMode::Max)
		m_eAutoNextMode = AutoNextMode::Min;
	return m_eAutoNextMode;
}

BOOL CPlayer::LrcUpdatePosition()
{
	if (!m_pLrc)
		return FALSE;
	const auto idxNew = m_pLrc->MgTimeToLine((float)m_lfCurrTime, m_idxCurrLrc);
	if (m_idxCurrLrc != idxNew)
	{
		m_idxCurrLrc = idxNew;
		return TRUE;
	}
	else
		return FALSE;
}