#pragma once

#include "timeout.h"
#include "remote_node_proxy.h"

#include "state_init.h"
#include "state_follower.h"
#include "state_candidate.h"
#include "state_leader.h"

#include <random>
#include <memory>
#include <vector>
#include <algorithm>

#include <iostream>

using namespace std::chrono;

class LocalNode : public Timeout::EventHandler
{
	friend State;
	friend StateInit;
	friend StateFollower;
	friend StateCandidate;
	friend StateLeader;

public:
    LocalNode(NodeId id,
	          std::shared_ptr<Timeout> timeout,
	          milliseconds const& el_timeout_min,
	          milliseconds const& el_timeout_max)
    : Timeout::EventHandler()
	, _rd()
	, _mt(_rd())
	, _timeout(std::move(timeout))
	, _timeout_span(el_timeout_min.count(), el_timeout_max.count())
	, _id(id)
	, _current_term(0)
	, _voted_for(0)
	, _log()
	, _commit_index(0)
	, _last_applied(0)
	, _remote_nodes()
	, _state_init()
	, _state_follower()
	, _state_candidate()
	, _state_leader()
	, _state(&_state_init)
    {
        _timeout->handler(this);

		_change_state(_state_follower);
    }

    void add(std::shared_ptr<RemoteNode> remote_node,
	         std::shared_ptr<Timeout> timeout)
    {
        _remote_nodes.emplace_back(
			*this,
			std::move(remote_node),
			std::move(timeout),
			milliseconds(_timeout_span.min()/2));
    }

	NodeId id() const
	{
		return _id;
	}

	Log const& log() const
	{
		return _log;
	}

	Log& log()
	{
		return _log;
	}

	Term current_term() const
	{
		return _current_term;
	}

	LogIndex commit_index() const
	{
		return _commit_index;
	}

	bool has_voted() const
	{
		return _voted_for != 0;
	}

	NodeId voted_for() const
	{
		return _voted_for;
	}

	bool is_follower() const
	{
		return _state == &_state_follower;
	}

	bool is_candidate() const
	{
		return _state == &_state_candidate;
	}

	bool is_leader() const
	{
		return _state == &_state_leader;
	}

	void append_entries(
			LogIndex prev_log_idx,
			Log const& entries,
			LogIndex commit_idx)
	{
		auto loc_log_it = _log.begin() + prev_log_idx;
		auto rem_log_it = entries.begin();

		while (    loc_log_it != _log.end()
			   and rem_log_it != entries.end())
		{
			if ((*loc_log_it).first != (*rem_log_it).first)
			{
				_log.erase(loc_log_it, _log.end());
				break;
			}

			++loc_log_it;
			++rem_log_it;
		}

		std::copy(rem_log_it,
				  entries.end(),
				  std::back_inserter(_log));

		if (commit_idx > _commit_index)
			_commit_index = std::min(commit_idx, _log.size());
	}

	void request_vote(NodeId id)
	{
		_voted_for = id;
	}

	void higher_term_detected(Term higher_term)
	{
		_state->new_term_detected(*this, higher_term);
	}

	void vote_granted()
	{
		if (_is_vote_majority_reached())
			_state->majority_detected(*this);
	}

    // Timeout::EventHandler ///////////////////////////////////////////////////

    void timeout() override
    {
		_state->timeout(*this);
    }

    ////////////////////////////////////////////////////////////////////////////

private:
	void _change_state(State& next_state)
	{
		if (_state == &next_state)
			return;

		_state->leave(*this);
		_state = &next_state;
		_state->enter(*this);
	}

	void _restart_timeout()
	{
		_timeout->reset(milliseconds(_timeout_span(_mt)));
	}

	bool _is_vote_majority_reached()
	{
		auto nr_votes = std::count_if(
			_remote_nodes.begin(),
			_remote_nodes.end(),
			[](auto const& node)
			{ return node.is_vote_granted(); });

		return nr_votes > ((_remote_nodes.size() + 1) / 2);
	}

private:
	std::random_device                               _rd;
	std::mt19937                                     _mt;

    std::shared_ptr<Timeout>                         _timeout;
	std::uniform_int_distribution<milliseconds::rep> _timeout_span;

	NodeId                                           _id;
    Term                                             _current_term;
    NodeId                                           _voted_for;
    Log                                              _log;

    LogIndex                                         _commit_index;
    LogIndex                                         _last_applied;

    std::vector<RemoteNodeProxy>                     _remote_nodes;

	StateInit                                        _state_init;
	StateFollower                                    _state_follower;
	StateCandidate                                   _state_candidate;
	StateLeader                                      _state_leader;
	State*                                           _state;
};
