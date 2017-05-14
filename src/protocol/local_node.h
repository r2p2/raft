#pragma once

#include "timeout.h"
#include "remote_node.h"

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

class LocalNode : public RemoteNode::RequestHandler,
                  public Timeout::EventHandler
{
	friend State;
	friend StateInit;
	friend StateFollower;
	friend StateCandidate;
	friend StateLeader;

	struct RemoteNodeData // TODO eliminate stupid data; make intelligent proxy
	{
		RemoteNodeData(std::shared_ptr<RemoteNode> node_ptr)
		: node_ptr(std::move(node_ptr))
		{
		}

		void reset(LocalNode& node)
		{
			next_index = node.log().size() + 1;
			match_index = 0;
			voted_for_me = false;
		}

		std::shared_ptr<RemoteNode> node_ptr;
		LogIndex                    next_index   = 0;
		LogIndex                    match_index  = 0;
		bool                        voted_for_me = false;
	};

public:
    LocalNode(NodeId id,
	          std::shared_ptr<Timeout> timeout,
	          milliseconds const& el_timeout_min,
	          milliseconds const& el_timeout_max)
    : _rd()
	, _mt(_rd())
	, _timeout(std::move(timeout))
	, _timeout_span(el_timeout_min.count(), el_timeout_max.count())
	, _id(id)
	, _current_term(0)
	, _voted_for(0)
	, _log()
	, _commit_index(0)
	, _last_applied(0)
	, _nodes()
	, _state_init()
	, _state_follower()
	, _state_candidate()
	, _state_leader()
	, _state(&_state_init)
    {
        _timeout->handler(this);

		_change_state(_state_follower);
    }

    void add(std::shared_ptr<RemoteNode> node_ptr)
    {
		node_ptr->handler(this);
        _nodes.emplace_back(std::move(node_ptr));
    }

	NodeId id() const
	{
		return _id;
	}

	Log const& log() const
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

    // RemoteNode::RequestHandler //////////////////////////////////////////////

    std::pair<Term, bool> append_entries(AppendEntryArguments const& args) override
    {
		if (_current_term > args.leaders_term)
			return {_current_term, false};

		if (_current_term < args.leaders_term)
			_state->new_term_detected(*this, args.leaders_term);

		if (    args.prev_log_index > 0
		    and _log.size() >= args.prev_log_index
		    and _log[args.prev_log_index-1].first != args.prev_log_term)
		{
			return {_current_term, false};
		}

		auto loc_log_it = _log.begin() + args.prev_log_index;
		auto rem_log_it = args.entries.begin();

		while (    loc_log_it != _log.end()
		       and rem_log_it != args.entries.end())
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
		          args.entries.end(),
		          std::back_inserter(_log));

		if (args.leaders_commit_index > _commit_index)
			_commit_index = std::min(args.leaders_commit_index, _log.size());

		return {_current_term, true};
    }

    std::pair<Term, bool> request_vote(VoteRequestArguments const& args) override
    {
		if (_current_term > args.candidates_term)
			return {_current_term, false};

		if (args.candidates_term > _current_term)
			_state->new_term_detected(*this, args.candidates_term);

		if (has_voted() and voted_for() != args.candidates_id)
			return {_current_term, false};

		if (args.candidates_last_log_entry < _log.size())
			return {_current_term, false};

		_voted_for = args.candidates_id;

		return {_current_term, true};
    }

    void append_entries_reply(RemoteNode& src,
	                          Term current_term,
	                          bool success) override
    {
		if (_current_term < current_term)
		{
			_state->new_term_detected(*this, current_term);
			return;
		}

		if (is_leader()) // TODO extract into leader state model
		{
			auto rem_node_it = std::find_if(
				_nodes.begin(),
				_nodes.end(),
				[&src](auto const& node_data)
				{
					return node_data.node_ptr.get() == &src;
				});

			auto& rem_node = *rem_node_it;

			if (log().size() < rem_node.next_index)
				return;

			auto ae = AppendEntryArguments(_current_term,
										   id(),
										   rem_node.next_index - 1,
										   0,
										   {},
										   commit_index());

			if ((rem_node.next_index - 1) > 0)
				ae.prev_log_term = log()[rem_node.next_index - 2].first;

			rem_node.node_ptr->append_entries(ae);

		}
    }

    void request_vote_reply(RemoteNode& src,
	                        Term current_term,
	                        bool granted) override
    {
		if (_current_term < current_term)
		{
			_state->new_term_detected(*this, current_term);
			return;
		}

		if (not granted)
			return;

		for (auto& rem_node : _nodes)
		{
			if (rem_node.node_ptr.get() == &src)
			{
				rem_node.voted_for_me = true;
				break;
			}
		}

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

	void _restart_heartbeat()
	{
		_timeout->reset(milliseconds(_timeout_span.min()/2));
	}

	bool _is_vote_majority_reached()
	{
		auto nr_votes = std::count_if(_nodes.begin(), _nodes.end(),
			[](auto const& node)
			{
				return node.voted_for_me;
			});

		return nr_votes > ((_nodes.size() + 1) / 2);
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

    std::vector<RemoteNodeData>                      _nodes;

	StateInit                                        _state_init;
	StateFollower                                    _state_follower;
	StateCandidate                                   _state_candidate;
	StateLeader                                      _state_leader;
	State*                                           _state;
};
