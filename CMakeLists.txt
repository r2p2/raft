cmake_minimum_required (VERSION 3.0)

project (raft)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_definitions(
	-Wfatal-errors
)

include_directories (
	PUBLIC src
)

set (SRC_FILES
	src/protocol/append_entry_arguments.cc
	src/protocol/state.cc
	src/protocol/state_candidate.cc
	src/protocol/state_follower.cc
	src/protocol/state_leader.cc
	src/protocol/vote_request_arguments.cc
)

add_executable (raft-test
	${SRC_FILES}
	tests/main.cc
)

target_include_directories (raft-test
	PUBLIC libs/catch
	PUBLIC libs/fakeit/single_header/catch
)

