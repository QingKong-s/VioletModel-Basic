#pragma once

class CPlayList
{
private:
	struct FLAT
	{
		int idxInPool;
	};
	struct GROUPSUB
	{
		int idxInPool;
	};
	struct GROUP
	{
		eck::CRefStrW rsName{};
		std::vector<GROUPSUB> vItem{};
	};

	std::vector<PLLITEM> m_vItemPool{};
	std::vector<FLAT> m_vFlat{};
	std::vector<GROUP> m_vGroup{};

	int m_idxCurrFlat{};
	int m_idxCurrGroup{};
	int m_idxCurrGroupItem{};
	int m_idxFirstFree{ -1 };

	BITBOOL m_bEnableGroup : 1{};

	PLLITEM& PoolAllocItem(_Out_ int& idx);
public:
	EckInlineNdCe auto& FlAt(int idx) noexcept { return m_vItemPool[m_vFlat[idx].idxInPool]; }
	EckInlineNdCe auto& GrAtGroup(int idxGroup) noexcept { return m_vGroup[idxGroup]; }
	EckInlineNdCe auto& GrAt(int idxGroup, int idxItem) noexcept { return m_vItemPool[m_vGroup[idxGroup].vItem[idxItem].idxInPool]; }

	BOOL IsActive() noexcept;

	EckInlineCe void SetCurrentItem(int idx) noexcept { m_idxCurrFlat = idx; }
	EckInlineCe void SetCurrentItem(int idxGroup, int idxItem) noexcept
	{
		m_idxCurrGroup = idxGroup;
		m_idxCurrGroupItem = idxItem;
	}

	EckInlineCe void EnableGroup(BOOL b) noexcept { m_bEnableGroup = b; }
	EckInlineNdCe BOOL IsGroupEnabled() const noexcept { return m_bEnableGroup; }

	int FlInsert(const eck::CRefStrW& rsFile, int idx = -1);
};