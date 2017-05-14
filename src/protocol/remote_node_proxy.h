#pragma once

#include "timeout.h"
#include "remote_node.h"

#include <memory>

class LocalNode;
class RemoteNode;

class RemoteNodeProxy : public RemoteNode::RequestHandler
                      , public Timeout::EventHandler
{
	enum class State
	{
		Idle,
		Sync,
		Vote
	};

public:
	RemoteNodeProxy(LocalNode& local_node,
	                std::shared_ptr<RemoteNode> remote_node,
	                std::shared_ptr<Timeout> timeout,
	                std::chrono::milliseconds hb_duration);

	void start_election_cycle();
	bool is_vote_granted() const;

    // RemoteNode::RequestHandler //////////////////////////////////////////////
    std::pair<Term, bool> append_entries(AppendEntryArguments const& args) override;

    std::pair<Term, bool> request_vote(VoteRequestArguments const& args) override;

    void append_entries_reply(RemoteNode& src,
	                          Term current_term,
	                          bool success) override;

    void request_vote_reply(RemoteNode& src,
	                        Term current_term,
	                        bool granted) override;

    // Timeout::EventHandler ///////////////////////////////////////////////////

    void timeout() override;

    ////////////////////////////////////////////////////////////////////////////

private:
	void _heartbeat();
	void _sync();
	void _vote();
	void _reset_timeout();

private:
	LocalNode&                  _local_node;
	std::shared_ptr<RemoteNode> _remote_node;
	std::shared_ptr<Timeout>    _timeout;
	std::chrono::milliseconds   _hb_duration;

	State                       _state        = State::Idle;
	LogIndex                    _next_index   = 0;
	LogIndex                    _match_index  = 0;
	bool                        _vote_granted = false;
};
