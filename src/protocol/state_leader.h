#pragma once

#include "state.h"

class StateLeader : public State
{
public:
	std::string name() const override
	{
		return "leader";
	}

	void enter(LocalNode&) override;

	void timeout(LocalNode&) override;

private:
	void _send_heartbeats(LocalNode&);
};
