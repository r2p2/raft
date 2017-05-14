#include "vote_request_arguments.h"

bool operator== (VoteRequestArguments const& a, VoteRequestArguments const& b)
{
	return a.candidates_term           == b.candidates_term
	   and a.candidates_id             == b.candidates_id
	   and a.candidates_last_log_entry == b.candidates_last_log_entry
	   and a.candidates_last_log_term  == b.candidates_last_log_term;
}
