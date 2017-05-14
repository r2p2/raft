#pragma once

#include "state.h"

class StateInit : public State
{
	std::string name() const override
	{
		return "init";
	}
};
