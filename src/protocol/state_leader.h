#pragma once

#include "state.h"

class StateLeader : public State
{
public:
	std::string name() const override
	{
		return "leader";
	}
};
