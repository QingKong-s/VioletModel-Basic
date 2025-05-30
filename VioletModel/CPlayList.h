#pragma once

class CPlayList
{
	friend class CPlayer;
public:
	struct GROUPIDX
	{
		int idxGroup;
		int idxItem;
	};
private:
	struct ITEM
	{
		eck::CRefStrW rsName{};		// 名称
		eck::CRefStrW rsFile{};		// 文件路径

		eck::CRefStrW rsTitle{};	// 标题
		eck::CRefStrW rsArtist{};	// 艺术家
		eck::CRefStrW rsAlbum{};	// 唱片集
		eck::CRefStrW rsGenre{};	// 流派

		PLDATA s{};
		union
		{
			int idxSortMapping{ -1 };// 【排序时用】映射到的项
			int idxNextFree;		// 【仅当项目空闲时】下一个空闲项
		};
		ULONGLONG TskTag{};			// 与该项关联的信息获取任务
		int idxIl{ 0 };				// 图像列表索引，供UI使用
	};
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

	std::vector<ITEM> m_vItemPool{};
	std::vector<FLAT> m_vFlat{};
	std::vector<GROUP> m_vGroup{};

	int m_idxCurrFlat{};
	int m_idxCurrGroup{};
	int m_idxCurrGroupItem{};
	int m_idxFirstFree{ -1 };

	BITBOOL m_bGroup : 1{};
	BITBOOL m_bSort : 1{};
	PlType m_eType{};

	std::atomic_bool m_bTaskRunning{};

	ITEM& ImAllocItem(_Out_ int& idx);
public:
	ITEM* FindTag(ULONGLONG TskTag) noexcept;

	EckInlineNdCe auto& FlAt(int idx) noexcept { return m_vItemPool[m_vFlat[idx].idxInPool]; }

	/// <summary>
	/// 插入项目。
	/// 主操作列表为平面列表
	/// </summary>
	/// <param name="rsFile">文件全路径</param>
	/// <param name="idx">插入位置，-1表示末尾</param>
	/// <returns>索引</returns>
	int FlInsert(const eck::CRefStrW& rsFile, int idx = -1, ULONGLONG TskTag = 0);

	EckInlineNdCe int FlGetCount() const noexcept { return (int)m_vFlat.size(); }

	EckInlineNdCe auto& GrAtGroup(int idxGroup) noexcept { return m_vGroup[idxGroup]; }
	EckInlineNdCe auto& GrAt(int idxGroup, int idxItem) noexcept { return m_vItemPool[m_vGroup[idxGroup].vItem[idxItem].idxInPool]; }

	/// <summary>
	/// 插入组
	/// </summary>
	/// <param name="rsName">组名</param>
	/// <param name="idx">插入位置，-1表示末尾</param>
	/// <returns>组索引</returns>
	int GrInsertGroup(const eck::CRefStrW& rsName, int idx = -1);

	/// <summary>
	/// 插入项目。
	/// 主操作列表为分组列表
	/// </summary>
	/// <param name="rsFile">文件全路径</param>
	/// <param name="idxItem">项目在组中的索引，-1表示末尾</param>
	/// <param name="idxGroup">所在组的索引，-1表示新建组</param>
	/// <returns></returns>
	GROUPIDX GrInsert(const eck::CRefStrW& rsFile, int idxItem = -1, int idxGroup = -1);

	// 当前列表是否已被播放器选入
	BOOL IsActive() noexcept;

	// ---下列方法仅供播放器使用---

	EckInlineCe void PlySetCurrentItem(int idx) noexcept { m_idxCurrFlat = idx; }
	EckInlineCe void PlySetCurrentItem(int idxGroup, int idxItem) noexcept
	{
		EckAssert((idxGroup < 0) ? (idxItem < 0) : TRUE);
		m_idxCurrGroup = idxGroup;
		m_idxCurrGroupItem = idxItem;
	}
	// -------------------------

	EckInlineCe void EnableGroup(BOOL b) noexcept { m_bGroup = b; }
	EckInlineNdCe BOOL IsGroupEnabled() const noexcept { return m_bGroup; }

	HRESULT InitFromListFile(PCWSTR pszFile);

	EckInline void SetTaskRunning(bool b) noexcept { m_bTaskRunning = b; }
	EckInlineNd bool GetTaskRunning() const noexcept { return m_bTaskRunning; }
};