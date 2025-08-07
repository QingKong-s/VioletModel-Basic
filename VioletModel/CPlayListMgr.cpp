#include "pch.h"
#include "CPlayListMgr.h"

void CPlayListMgr::LoadList()
{
	auto rsPath{ eck::GetRunningPath() };
	rsPath.PushBack(L"\\List");
	auto FnCallback = [&](eck::CEnumFile::TDefInfo& e)
		{
			auto& List = *m_vPlayList.emplace_back(std::make_shared<CPlayList>()).pList;
			List.SetListFile(rsPath.ToStringView(),
				std::wstring_view{ e.FileName,e.FileNameLength / sizeof(WCHAR) });
			List.ImMarkNeedInit();
		};
	eck::CEnumFile ef{ rsPath.Data() };
	ef.Enumerate(EckStrAndLen(L"*.VltList"), FnCallback);
	ef.Enumerate(EckStrAndLen(L"*.PNList"), FnCallback);
	ef.Enumerate(EckStrAndLen(L"*.QKList"), FnCallback);
}

std::shared_ptr<CPlayList> CPlayListMgr::Add()
{
	return m_vPlayList.emplace_back(std::make_shared<CPlayList>()).pList;
}

void CPlayListMgr::InvalidateImageList()
{
	for (auto& e : m_vPlayList)
	{
		e.pImageList.Clear();
		e.pList->InvalidateImage();
	}
}