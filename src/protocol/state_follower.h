#pragma once

#include "state.h"

class StateFollower : public State
{
	std::string name() const override
	{
		return "follower";
	}

	void enter(LocalNode&) override;

	void timeout(LocalNode&) override;
};
