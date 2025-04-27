#include "pch.h"
#include "CPlayList.h"
#include "CApp.h"

CPlayList::ITEM& CPlayList::ImAllocItem(_Out_ int& idx)
{
	if (m_idxFirstFree >= 0)
	{
		idx = m_idxFirstFree;
		auto& e = m_vItemPool[idx];
		m_idxFirstFree = e.idxNextFree;
		e.idxNextFree = -1;
		e.s.bFree = FALSE;
		return e;
	}
	else
	{
		idx = (int)m_vItemPool.size();
		return m_vItemPool.emplace_back();
	}
}

CPlayList::GROUPIDX CPlayList::GrInsert(const eck::CRefStrW& rsFile, int idxItem, int idxGroup)
{
	return GROUPIDX();
}

BOOL CPlayList::IsActive() noexcept
{
	return App->GetPlayer().GetList() == this;
}

HRESULT CPlayList::InitFromListFile(PCWSTR pszFile)
{
	const auto rbFile = eck::ReadInFile(pszFile);
	// TODO
	return S_OK;
}

int CPlayList::FlInsert(const eck::CRefStrW& rsFile, int idx)
{
	int idxPool;
	auto& e = ImAllocItem(idxPool);
	e.rsFile = rsFile;
	if (idx < 0)
	{
		m_vFlat.emplace_back(idxPool);
		idx = (int)m_vFlat.size() - 1;
	}
	else
		m_vFlat.emplace(m_vFlat.begin() + idx, idxPool);
	return idx;
}

int CPlayList::GrInsertGroup(const eck::CRefStrW& rsName, int idx)
{
	return 0;
}
