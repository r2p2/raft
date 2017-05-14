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

	for (auto rem_node : node._nodes)
		rem_node.reset(node);

	_send_vote_requests(node);
}

void StateCandidate::_send_vote_requests(LocalNode& node)
{
	auto vra = VoteRequestArguments(node.current_term(),
	                                node.id(),
	                                node.log().size(),
	                                0);
	if (not node.log().empty())
		vra.candidates_last_log_term = node.log().back().first;

	for (auto rem_node : node._nodes)
		if (not rem_node.voted_for_me)
			rem_node.node_ptr->request_vote(vra);
}


