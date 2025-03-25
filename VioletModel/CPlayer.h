#pragma once
enum class PlayEvt
{

};

class CPlayer
{
private:
	eck::CSignal<eck::NoIntercept_T, void, PlayEvt, void*> m_Sig{};
};