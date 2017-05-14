#pragma once

#include "append_entry_arguments.h"
#include "vote_request_arguments.h"

class RemoteNode
{
public:
    class RequestHandler
    {
    public:
        virtual ~RequestHandler() = default;

        virtual std::pair<Term, bool> append_entries(AppendEntryArguments const&) = 0;

        virtual std::pair<Term, bool> request_vote(VoteRequestArguments const&) = 0;

        virtual void append_entries_reply(RemoteNode& src,
		                                  Term current_term,
		                                  bool success) = 0;

        virtual void request_vote_reply(RemoteNode& src,
		                                Term current_term,
		                                bool granted) = 0;
    };

    virtual ~RemoteNode() = default;

    virtual void handler(RequestHandler* handler) = 0;

    virtual NodeId id() const = 0;

    virtual void append_entries(AppendEntryArguments const&) = 0;

    virtual void request_vote(VoteRequestArguments const&) = 0;
};
