#pragma once
enum class PlEvt
{

};

class CPlayer
{
private:
	eck::CSignal<eck::NoIntercept_T, void, PlEvt, void*> m_Sig{};
};