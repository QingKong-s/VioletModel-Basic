#pragma once
#include "eck\PchInc.h"
#include "eck\SystemHelper.h"
#include "eck\CCommDlg.h"
#include "eck\CLinearLayout.h"
#include "eck\CLayoutDummy.h"
#include "eck\CDuiButton.h"
#include "eck\CDuiCircleButton.h"
#include "eck\CDuiEdit.h"
#include "eck\CDuiTrackBar.h"
#include "eck\CDuiTitleBar.h"
#include "eck\CDuiLabel.h"
#include "eck\CDuiTabList.h"
#include "eck\CDuiList.h"
#include "eck\CDuiGroupList.h"

using eck::PCVOID;
using eck::PCBYTE;
using eck::SafeRelease;

namespace Dui = eck::Dui;

enum
{
	ELEN_PLACEHOLDER = Dui::EE_PRIVATE_BEGIN,
	ELEN_PAGE_CHANGE,// [CTabPanel]边栏被单击时(LTN_ITEM*)

};

#include "CApp.h"
#include "CTabPanel.h"
#include "CPlayPanel.h"