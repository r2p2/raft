#include "state_leader.h"

#include "local_node.h"

void StateLeader::enter(LocalNode& node)
{
	_send_heartbeats(node);
}

void StateLeader::timeout(LocalNode& node)
{
	_send_heartbeats(node);
}

void StateLeader::_send_heartbeats(LocalNode& node)
{
	auto hb = AppendEntryArguments(node.current_term(),
	                               node.id(),
	                               node.log().size(),
	                               0,
	                               {},
	                               node.commit_index());

	if (not node.log().empty())
		hb.prev_log_term = node.log().back().first;

	for (auto& rem_node : node._nodes)
		rem_node.node_ptr->append_entries(hb);


	node._restart_heartbeat();
}
