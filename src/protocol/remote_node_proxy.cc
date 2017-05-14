#include "remote_node_proxy.h"

#include "local_node.h"

RemoteNodeProxy::RemoteNodeProxy(LocalNode& local_node,
                                 std::shared_ptr<RemoteNode> remote_node,
                                 std::shared_ptr<Timeout> timeout,
                                 std::chrono::milliseconds hb_duration)
: _local_node(local_node)
, _remote_node(std::move(remote_node))
, _timeout(std::move(timeout))
, _hb_duration(std::move(hb_duration))
{
	_remote_node->handler(this);
	_timeout->handler(this);
}

void RemoteNodeProxy::start_election_cycle()
{
	_state         = State::Idle;
	_next_index    = _local_node.log().size() + 1;
	_match_index   = 0;
	_vote_granted = false;

	_vote();
}

bool RemoteNodeProxy::is_vote_granted() const
{
	return _vote_granted;
}

std::pair<Term, bool> RemoteNodeProxy::append_entries(AppendEntryArguments const& args)
{
	bool const old_term = _local_node.current_term() > args.leaders_term;
	bool const new_term = _local_node.current_term() < args.leaders_term;
	bool const inv_data =
			args.prev_log_index > 0 and
			_local_node.log().size() >= args.prev_log_index and
			_local_node.log()[args.prev_log_index - 1].first != args.prev_log_term;

	if (new_term)
		_local_node.higher_term_detected(args.leaders_term);

	if (old_term or new_term or inv_data)
		return {_local_node.current_term(), false};

	_local_node.append_entries(
			args.prev_log_index,
			args.entries,
			args.leaders_commit_index);

	return {_local_node.current_term(), true};
}


std::pair<Term, bool> RemoteNodeProxy::request_vote(VoteRequestArguments const& args)
{
	bool const old_term = _local_node.current_term() > args.candidates_term;
	bool const new_term = _local_node.current_term() < args.candidates_term;
	bool const inv_vote =
			_local_node.has_voted() and
			_local_node.voted_for() != args.candidates_id;
	bool const inv_data =
			_local_node.log().size() >= args.candidates_last_log_entry;

	if (new_term)
		_local_node.higher_term_detected(args.candidates_term);

	if (old_term or new_term or inv_vote or inv_data)
		return {_local_node.current_term(), false};

	_local_node.request_vote(args.candidates_id);

	return {_local_node.current_term(), true};
}

void RemoteNodeProxy::append_entries_reply(RemoteNode& src,
                                           Term current_term,
                                           bool success)
{
	bool const old_term = _local_node.current_term() > current_term;
	bool const new_term = _local_node.current_term() < current_term;
	bool const not_lead = not _local_node.is_leader();

	if (new_term)
		_local_node.higher_term_detected(current_term);

	if (old_term or new_term or not_lead)
		return;

	if (_local_node.log().size() < _next_index)
	{
		_state = State::Idle;
		_reset_timeout();
		return;
	}

	_sync();
}


void RemoteNodeProxy::request_vote_reply(RemoteNode& /*src*/,
                                         Term current_term,
                                         bool granted)
{
	bool const old_term = _local_node.current_term() > current_term;
	bool const new_term = _local_node.current_term() < current_term;

	if (new_term)
		_local_node.higher_term_detected(current_term);

	if (new_term or old_term)
		return;

	_vote_granted = granted;

	if (granted)
		_local_node.vote_granted();
}

void RemoteNodeProxy::timeout()
{
	switch (_state)
	{
	case State::Idle:
		_heartbeat();
		break;
	case State::Sync:
		_sync();
		break;
	case State::Vote:
		_vote();
		break;
	}

	_reset_timeout();
}

void RemoteNodeProxy::_heartbeat()
{
	if (not _local_node.is_leader())
		return;

	_state = State::Idle;

	auto const hb = AppendEntryArguments(
		_local_node.current_term(),
		_local_node.id(),
		_local_node.log().size(),
		_local_node.log().empty() ? 0 : _local_node.log().back().first,
		{},
		_local_node.commit_index());

	_remote_node->append_entries(hb);
}

void RemoteNodeProxy::_sync()
{
	if (not _local_node.is_leader())
		return;

	_state = State::Sync;

	auto const& log         = _local_node.log();
	auto const prev_log_idx = _next_index - 1;
	auto const prev_log_trm =
		log.size() < prev_log_idx ? 0
	                              : log[prev_log_idx - 1].first;

	Log sync_log;
	std::copy(log.begin() + (prev_log_idx - 1),
	          log.end(),
	          std::back_inserter(sync_log));

	auto const ar = AppendEntryArguments(
		_local_node.current_term(),
		_local_node.id(),
		prev_log_idx,
		prev_log_trm,
		sync_log,
		_local_node.commit_index());

	_remote_node->append_entries(ar);
}


void RemoteNodeProxy::_vote()
{
	if (not _local_node.is_candidate())
		return;

	_state = State::Vote;

	auto const& log = _local_node.log();
	auto const vr = VoteRequestArguments(
		_local_node.current_term(),
	    _local_node.id(),
		log.size(),
		log.empty() ? 0 : log.back().first);

	_remote_node->request_vote(vr);
}

void RemoteNodeProxy::_reset_timeout()
{
	_timeout->reset(_hb_duration);
}
