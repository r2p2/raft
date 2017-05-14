#pragma once

#include <cstddef>
#include <utility>
#include <vector>

typedef int                         Term;
typedef int                         NodeId;

typedef int                         LogElement;
typedef size_t                      LogIndex;
typedef std::pair<Term, LogElement> LogEntry; // TODO to struct
typedef std::vector<LogEntry>       Log;
