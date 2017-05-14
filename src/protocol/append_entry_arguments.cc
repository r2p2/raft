#include "append_entry_arguments.h"

bool operator== (AppendEntryArguments const& a, AppendEntryArguments const& b)
{
	return a.leaders_term         == b.leaders_term
	   and a.leader_id            == b.leader_id
	   and a.prev_log_index       == b.prev_log_index
	   and a.prev_log_term        == b.prev_log_term
	   and a.entries              == b.entries
	   and a.leaders_commit_index == b.leaders_commit_index;
}
