#pragma once
#include "CVeCover.h"
#include "CVeLrc.h"
class CPagePlaying : public Dui::CElem
{
private:
	CVeCover m_Cover{};
	CVeLrc m_Lrc{};
};