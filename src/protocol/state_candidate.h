#pragma once

#include "state.h"

class StateCandidate : public State
{
public:
	std::string name() const override
	{
		return "canidate";
	}

	void enter(LocalNode&) override;

	void majority_detected(LocalNode&) override;

	void timeout(LocalNode&) override;

private:
	void _new_election(LocalNode&);
};
