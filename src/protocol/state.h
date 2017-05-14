#pragma once

#include "append_entry_arguments.h"
#include "vote_request_arguments.h"

#include <string>

class LocalNode;
struct RemoteNodeData;

class State
{
public:
	virtual ~State() = default;

	virtual std::string name() const = 0;

	virtual void timeout(LocalNode&) {}

	virtual void enter(LocalNode&) {}
	virtual void leave(LocalNode&) {}

	virtual void new_term_detected(LocalNode&, Term new_term);
	virtual void majority_detected(LocalNode&) {};
};
