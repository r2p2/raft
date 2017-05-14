#pragma once

#include <protocol/local_node.h>

#include <fakeit.hpp>

using namespace std::chrono_literals;
using namespace fakeit;

TEST_CASE("FOLLOWER", "LOCAL")
{
	auto const el_timeout_min{150ms};
	auto const el_timeout_max{300ms};

	std::vector<milliseconds> timeouts;

	Mock<Timeout> mock_timeout;
	Fake(Dtor(mock_timeout));
	Fake(Method(mock_timeout, handler));
	Fake(Method(mock_timeout, reset));

	When(Method(mock_timeout, reset)).AlwaysDo(
		[&timeouts](auto const& duration)
		{
			timeouts.push_back(duration);
		});

	LocalNode node(1,
	               std::shared_ptr<Timeout>(&mock_timeout.get()),
	               el_timeout_min,
	               el_timeout_max);

	SECTION("new node starts as follower")
	{
		REQUIRE(node.is_follower() == true);
	}

	SECTION("new node starts with term == 0")
	{
		REQUIRE(node.current_term() == 0);
	}

	SECTION("new node starts with commit_index == 0")
	{
		REQUIRE(node.commit_index() == 0);
	}

	SECTION("new node starts with an empty log")
	{
		REQUIRE(node.log().empty() == true);
	}

	SECTION("new node has not voted yet")
	{
		REQUIRE(node.has_voted() == false);
	}

	SECTION("new node registers itself to the timeout")
	{
		Verify(Method(mock_timeout, handler)).Exactly(Once);

	}

	SECTION("new node starts heartbeat timeout")
	{
		REQUIRE(timeouts.size() == 1);
		REQUIRE(timeouts.back() >= el_timeout_min);
		REQUIRE(timeouts.back() <= el_timeout_max);
	}

	SECTION("becomes candidate after timeout")
	{
		node.timeout();
		REQUIRE(node.is_candidate());
	}

	SECTION("grants first valid vote request in a term")
	{
		auto reply = node.request_vote({1, 2, 0, 0});
		REQUIRE(reply.first == 1);
		REQUIRE(reply.second == true);
		REQUIRE(node.current_term() == 1);
		REQUIRE(node.has_voted() == true);
		REQUIRE(node.voted_for() == 2);
	}

	SECTION("rejects second valid vote request in a term")
	{
		node.request_vote({1, 2, 0, 0});

		auto reply = node.request_vote({1, 3, 0, 0});
		REQUIRE(reply.first == 1);
		REQUIRE(reply.second == false);
		REQUIRE(node.current_term() == 1);
		REQUIRE(node.voted_for() == 2);
	}

	SECTION("grants second valid vote request from new term")
	{
		node.request_vote({1, 2, 0, 0});

		auto reply = node.request_vote({2, 3, 0, 0});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == true);
		REQUIRE(node.current_term() == 2);
		REQUIRE(node.voted_for() == 3);
	}

	SECTION("grants second valid vote request from same candidate")
	{
		node.request_vote({1, 2, 0, 0});

		auto reply = node.request_vote({1, 2, 0, 0});
		REQUIRE(reply.first == 1);
		REQUIRE(reply.second == true);
		REQUIRE(node.current_term() == 1);
		REQUIRE(node.voted_for() == 2);
	}

	SECTION("rejects vote request from older term")
	{
		node.request_vote({2, 2, 0, 0});

		auto reply = node.request_vote({1, 3, 0, 0});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == false);
		REQUIRE(node.current_term() == 2);
		REQUIRE(node.voted_for() == 2);
	}

	SECTION("rejects vote request due to old log entries")
	{
		node.request_vote({2, 2, 0, 0});
		node.append_entries({2, 2, 0, 0, {{1, 1}}, 1});
		REQUIRE(node.log().size() == 1);

		auto reply = node.request_vote({3, 3, 0, 0});
		REQUIRE(reply.first == 3);
		REQUIRE(reply.second == false);
		REQUIRE(node.current_term() == 3);
		REQUIRE(node.has_voted() == false);
	}

	SECTION("reject append entry request from older term")
	{
		node.request_vote({2, 2, 0, 0});

		auto reply = node.append_entries({1, 2, 1, 1, {}, 1});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == false);
		REQUIRE(node.commit_index() == 0);
	}

	SECTION("accept append entry request for single element on empty log")
	{
		node.request_vote({2, 2, 0, 0});

		auto reply = node.append_entries({2, 2, 0, 0, {{1, 1}}, 0});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == true);
		REQUIRE(node.log().empty() == false);
		REQUIRE(node.log().size() == 1);
		REQUIRE(node.log().back().first == 1);
		REQUIRE(node.log().back().second == 1);
		REQUIRE(node.commit_index() == 0);
	}

	SECTION("accept append entry request for single element on non empty log")
	{
		node.request_vote({2, 2, 0, 0});

		node.append_entries({2, 2, 0, 0, {{2, 1}}, 1});
		auto reply = node.append_entries({2, 2, 1, 2, {{2, 3}}, 1});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == true);
		REQUIRE(node.log().empty() == false);
		REQUIRE(node.log().size() == 2);
		REQUIRE(node.log().front().first == 2);
		REQUIRE(node.log().front().second == 1);
		REQUIRE(node.log().back().first == 2);
		REQUIRE(node.log().back().second == 3);
		REQUIRE(node.commit_index() == 1);
	}

	SECTION("accept append entry request for many elements with merge")
	{
		node.request_vote({2, 2, 0, 0});

		node.append_entries({2, 2, 0, 0, {{2, 1}}, 1});
		auto reply = node.append_entries({2, 2, 0, 0, {{2, 1}, {2, 3}}, 2});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == true);
		REQUIRE(node.log().empty() == false);
		REQUIRE(node.log().size() == 2);
		REQUIRE(node.log().back().first == 2);
		REQUIRE(node.log().back().second == 3);
		REQUIRE(node.commit_index() == 2);
	}

	SECTION("accept append entry request with too big commit index")
	{
		node.request_vote({2, 2, 0, 0});

		node.append_entries({2, 2, 0, 0, {{2, 1}}, 1});
		auto reply = node.append_entries({2, 2, 0, 0, {{2, 1}, {2, 3}}, 7});
		REQUIRE(reply.first == 2);
		REQUIRE(reply.second == true);
		REQUIRE(node.log().empty() == false);
		REQUIRE(node.log().size() == 2);
		REQUIRE(node.log().back().first == 2);
		REQUIRE(node.log().back().second == 3);
		REQUIRE(node.commit_index() == 2);
	}

	SECTION("accept append entry request with conflicting terms")
	{
		node.request_vote({1, 2, 0, 0});

		node.append_entries({1, 2, 0, 0, {{1, 1}}, 1});
		node.append_entries({2, 2, 1, 1, {{2, 1}}, 1});
		node.append_entries({3, 2, 2, 2, {{3, 1}}, 1});

		auto reply = node.append_entries({4, 2, 1, 1, {{2, 1}, {4, 1}, {4, 2}}, 2});

		REQUIRE(reply.first == 4);
		REQUIRE(reply.second == true);
		REQUIRE(node.log().empty() == false);
		REQUIRE(node.log().size() == 4);


		REQUIRE(node.log()[0].first  == 1);
		REQUIRE(node.log()[0].second == 1);

		REQUIRE(node.log()[1].first  == 2);
		REQUIRE(node.log()[1].second == 1);

		REQUIRE(node.log()[2].first  == 4);
		REQUIRE(node.log()[2].second == 1);

		REQUIRE(node.log()[3].first  == 4);
		REQUIRE(node.log()[3].second == 2);

		REQUIRE(node.commit_index() == 2);
	}

}
