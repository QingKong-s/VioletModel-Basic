#include "pch.h"
#include "CPlayListMgr.h"

void CPlayListMgr::LoadSavedLists()
{
	auto rsPath{ eck::GetRunningPath() };
	rsPath.PushBack(L"\\Lists");
	eck::CEnumFile ef{ rsPath.Data() };
	ef.Enumerate(EckStrAndLen(L"*.VlList"),
		[&](eck::CEnumFile::TDefInfo& e)
		{
		});
}
