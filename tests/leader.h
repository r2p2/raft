#pragma once

#include <protocol/local_node.h>

TEST_CASE("LEADER", "LOCAL")
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

	node.append_entries({2, 2, 0, 0, {{2, 1}}, 1});
	node.append_entries({2, 2, 1, 2, {{2, 3}}, 1});
	node.timeout();

	timeouts.clear();
	remote_2_append_entries.clear();
	remote_2_vote_requests.clear();

	remote_3_append_entries.clear();
	remote_3_vote_requests.clear();


	node.request_vote_reply(mock_remote_2.get(), 1, true);
	node.request_vote_reply(mock_remote_3.get(), 1, true);

	SECTION("a new leader has arrived")
	{
		REQUIRE(node.is_leader());
	}

	SECTION("a new leader has fast heartbeat timeouts")
	{
		REQUIRE(timeouts.size() == 1);
		REQUIRE(timeouts.back() == el_timeout_min/2);
	}

	SECTION("a new leader sends heartbeats after election")
	{
		auto const exp_hb = AppendEntryArguments(3,
		                                         1,
		                                         node.log().size(),
		                                         node.log().back().first,
		                                         {},
		                                         node.commit_index());
		REQUIRE(remote_2_append_entries.size() == 1);
		REQUIRE(remote_2_append_entries.back() == exp_hb);

		REQUIRE(remote_3_append_entries.size() == 1);
		REQUIRE(remote_3_append_entries.back().entries.empty() == true);
	}

	SECTION("sends heartbeats after (heartbeat) timeout")
	{
		node.timeout();

		REQUIRE(remote_2_append_entries.size() == 2);
		REQUIRE(remote_2_append_entries.back().entries.empty() == true);

		REQUIRE(remote_3_append_entries.size() == 2);
		REQUIRE(remote_3_append_entries.back().entries.empty() == true);
	}
}

