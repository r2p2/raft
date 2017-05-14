#pragma once

#include <protocol/local_node.h>

TEST_CASE("CANDIDATE", "LOCAL")
{
	std::vector<milliseconds> timeouts;

	std::vector<AppendEntryArguments> remote_2_append_entries;
	std::vector<VoteRequestArguments> remote_2_vote_requests;

	std::vector<AppendEntryArguments> remote_3_append_entries;
	std::vector<VoteRequestArguments> remote_3_vote_requests;

	auto const el_timeout_min{150ms};
	auto const el_timeout_max{300ms};

	Mock<Timeout> mock_timeout;
	Fake(Dtor(mock_timeout));
	Fake(Method(mock_timeout, handler));
	Fake(Method(mock_timeout, reset));

	When(Method(mock_timeout, reset)).AlwaysDo(
		[&timeouts](auto const& duration)
		{
			timeouts.push_back(duration);
		});


	Mock<RemoteNode> mock_remote_2;
	Fake(Dtor(mock_remote_2));
	Fake(Method(mock_remote_2, handler));
	Fake(Method(mock_remote_2, id));
	Fake(Method(mock_remote_2, append_entries));
	Fake(Method(mock_remote_2, request_vote));

	When(Method(mock_remote_2, id)).AlwaysReturn(2);
	When(Method(mock_remote_2, append_entries)).AlwaysDo(
		[&remote_2_append_entries](auto ae)
		{
			remote_2_append_entries.push_back(ae);
		});
	When(Method(mock_remote_2, request_vote)).AlwaysDo(
		[&remote_2_vote_requests](auto vr)
		{
			remote_2_vote_requests.push_back(vr);
		});


	Mock<RemoteNode> mock_remote_3;
	Fake(Dtor(mock_remote_3));
	Fake(Method(mock_remote_3, handler));
	Fake(Method(mock_remote_3, id));
	Fake(Method(mock_remote_3, append_entries));
	Fake(Method(mock_remote_3, request_vote));

	When(Method(mock_remote_3, id)).AlwaysReturn(3);
	When(Method(mock_remote_3, append_entries)).AlwaysDo(
		[&remote_3_append_entries](auto ae)
		{
			remote_3_append_entries.push_back(ae);
		});
	When(Method(mock_remote_3, request_vote)).AlwaysDo(
		[&remote_3_vote_requests](auto vr)
		{
			remote_3_vote_requests.push_back(vr);
		});


	LocalNode node(1,
	               std::shared_ptr<Timeout>(&mock_timeout.get()),
	               el_timeout_min,
	               el_timeout_max);

	node.add(std::shared_ptr<RemoteNode>(&mock_remote_2.get()));
	node.add(std::shared_ptr<RemoteNode>(&mock_remote_3.get()));

	timeouts.clear();
	node.timeout();

	SECTION("should have registered itself to remote nodes")
	{
		Verify(Method(mock_remote_2, handler)).Exactly(Once);
		Verify(Method(mock_remote_3, handler)).Exactly(Once);
	}

	SECTION("a fresh candidate should have reseted the election timer once")
	{
		REQUIRE(timeouts.size() == 1);
	}

	SECTION("should be in state candidate now")
	{
		REQUIRE(node.is_candidate() == true);
	}

	SECTION("a fresh candidate should have increased its term")
	{
		REQUIRE(node.current_term() == 1);
	}

	SECTION("a fresh candidate should have voted for itself")
	{
		REQUIRE(node.has_voted() == true);
		REQUIRE(node.voted_for() == 1);
	}

	SECTION("a fresh candidate requests votes")
	{
		REQUIRE(remote_2_append_entries.empty() == true);
		REQUIRE(remote_3_append_entries.empty() == true);

		REQUIRE(remote_2_vote_requests.size() == 1);
		REQUIRE(remote_3_vote_requests.size() == 1);

		auto const exp_vra = VoteRequestArguments(1, 1, 0, 0);
		REQUIRE(remote_2_vote_requests.back() == exp_vra);
		REQUIRE(remote_3_vote_requests.back() == exp_vra);
	}

	SECTION("starts new elections after every timeout")
	{
		REQUIRE(remote_2_vote_requests.size() == 1);
		REQUIRE(remote_3_vote_requests.size() == 1);

		node.timeout();

		REQUIRE(remote_2_vote_requests.size() == 2);
		REQUIRE(remote_3_vote_requests.size() == 2);

		node.timeout();

		REQUIRE(remote_2_vote_requests.size() == 3);
		REQUIRE(remote_3_vote_requests.size() == 3);
	}

	SECTION("becomes leader after majority of votes")
	{
		node.request_vote_reply(mock_remote_2.get(), 1, true);
		REQUIRE(node.is_leader() == false);

		node.request_vote_reply(mock_remote_3.get(), 1, true);
		REQUIRE(node.is_leader() == true);
	}

	SECTION("ignores redundant votes from same node")
	{
		node.request_vote_reply(mock_remote_2.get(), 1, true);
		REQUIRE(node.is_leader() == false);

		node.request_vote_reply(mock_remote_2.get(), 1, true);
		REQUIRE(node.is_leader() == false);
	}

	SECTION("ignores dismissed vote replies")
	{
		node.request_vote_reply(mock_remote_2.get(), 1, true);
		REQUIRE(node.is_leader() == false);

		node.request_vote_reply(mock_remote_3.get(), 1, false);
		REQUIRE(node.is_leader() == false);
	}

	SECTION("becomes follower when new term is detected in vote reply")
	{
		node.request_vote_reply(mock_remote_2.get(), 1, true);
		REQUIRE(node.is_leader() == false);

		node.request_vote_reply(mock_remote_3.get(), 2, false);
		REQUIRE(node.is_follower() == true);
	}

	SECTION("becomes follower when new term is detected in apend entries reply")
	{
		node.request_vote_reply(mock_remote_2.get(), 1, true);
		REQUIRE(node.is_leader() == false);

		node.append_entries_reply(mock_remote_3.get(), 2, false);
		REQUIRE(node.is_follower() == true);
	}
}
