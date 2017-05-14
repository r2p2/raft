#include "state_follower.h"

#include "local_node.h"

void StateFollower::enter(LocalNode& node)
{
	node._restart_timeout();
}

void StateFollower::timeout(LocalNode& node)
{
	node._change_state(node._state_candidate);
}
