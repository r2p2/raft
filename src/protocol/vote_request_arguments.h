#pragma once

#include "types.h"

struct VoteRequestArguments
{
	friend bool operator== (VoteRequestArguments const&, VoteRequestArguments const&);

	VoteRequestArguments(
			Term candidates_term,
			NodeId candidates_id,
			LogIndex candidates_last_log_entry,
			Term candidates_last_log_term)
	: candidates_term(candidates_term)
	, candidates_id(candidates_id)
	, candidates_last_log_entry(candidates_last_log_entry)
	, candidates_last_log_term(candidates_last_log_term)
	{
	}

	Term     candidates_term;
	NodeId   candidates_id;
	LogIndex candidates_last_log_entry;
	Term     candidates_last_log_term;
};

bool operator== (VoteRequestArguments const& a, VoteRequestArguments const& b);
