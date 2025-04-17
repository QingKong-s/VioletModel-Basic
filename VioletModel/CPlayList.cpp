#include "pch.h"
#include "CPlayList.h"
#include "CApp.h"

PLLITEM& CPlayList::PoolAllocItem(_Out_ int& idx)
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

BOOL CPlayList::IsActive() noexcept
{
	return App->GetPlayer().GetList() == this;
}

int CPlayList::FlInsert(const eck::CRefStrW& rsFile, int idx)
{
	int idxPool;
	auto& e = PoolAllocItem(idxPool);
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
