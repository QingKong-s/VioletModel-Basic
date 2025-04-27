#pragma once
#include "CPlayList.h"

class CPlayListMgr
{
private:
	std::vector<std::unique_ptr<CPlayList>> m_vPlayList{};
public:
	void LoadSavedLists();
};