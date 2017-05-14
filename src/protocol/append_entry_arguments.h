#pragma once

#include "types.h"

struct AppendEntryArguments
{
	friend bool operator== (AppendEntryArguments const&, AppendEntryArguments const&);

	AppendEntryArguments(
			Term     leaders_term,
			NodeId   leader_id,
			LogIndex prev_log_index,
			Term     prev_log_term,
			Log      entries,
			LogIndex leaders_commit_index)
	: leaders_term(leaders_term)
	, leader_id(leader_id)
	, prev_log_index(prev_log_index)
	, prev_log_term(prev_log_term)
	, entries(std::move(entries))
	, leaders_commit_index(leaders_commit_index)
	{
	}

	Term     leaders_term;
    NodeId   leader_id;
    LogIndex prev_log_index;
    Term     prev_log_term;
    Log      entries;
    LogIndex leaders_commit_index;
};

bool operator== (AppendEntryArguments const& a, AppendEntryArguments const& b);
