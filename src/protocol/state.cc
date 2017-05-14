#include "state.h"

#include "local_node.h"

void State::new_term_detected(LocalNode& node, Term new_term)
{
	node._current_term = new_term;
	node._voted_for    = 0;

	node._change_state(node._state_follower);
}
