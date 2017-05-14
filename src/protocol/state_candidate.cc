#include "state_candidate.h"

#include "local_node.h"

void StateCandidate::enter(LocalNode& node)
{
	State::enter(node);

	_new_election(node);
}

void StateCandidate::majority_detected(LocalNode& node)
{
	State::majority_detected(node);

	node._change_state(node._state_leader);
}

void StateCandidate::timeout(LocalNode& node)
{
	State::timeout(node);

	_new_election(node);
}

void StateCandidate::_new_election(LocalNode& node)
{
	node._restart_timeout();

	node._current_term += 1;
	node._voted_for = node._id;

	for (auto rem_node : node._remote_nodes)
		rem_node.start_election_cycle();
}

