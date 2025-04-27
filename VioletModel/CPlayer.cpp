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
		return PlayErr::BassError;
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
			return PlayErr::HResultError;
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
	Stop();
	GetList()->PlySetCurrentItem(idx);
	return PlayWorker(GetList()->FlAt(idx));
}

PlayErr CPlayer::Play(int idxGroup, int idxItem)
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	Stop();
	GetList()->PlySetCurrentItem(idxGroup, idxItem);
	return PlayWorker(GetList()->GrAt(idxGroup, idxItem));
}

PlayErr CPlayer::PlayOrPause()
{
	return PlayErr::Ok;
}

PlayErr CPlayer::Stop()
{
	if (!m_pPlayList)
		return PlayErr::NoPlayList;
	m_bActive = FALSE;
	m_Sig.Emit(PLAY_EVT_PARAM{ PlayEvt::Stop });
	m_Bass.Stop();
	m_Bass.Close();
	if (GetList()->IsGroupEnabled())
		GetList()->PlySetCurrentItem(-1, -1);
	else
		GetList()->PlySetCurrentItem(-1);
	return PlayErr::Ok;
}

void CPlayer::SetPosition(double lfPos)
{
	m_Bass.SetPosition(lfPos);
	m_lfCurrTime = m_Bass.GetPosition();
}
