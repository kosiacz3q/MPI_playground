#include "../catch/catch.hpp"

#include <Messages.hpp>

#include <utils.h>
#include <cstdio>


TEST_CASE( "sizeof char", "[environment]" )
{
	REQUIRE(sizeof(char) == 1);
}

TEST_CASE( "char to byte", "[utils]" )
{
	const char feed = 'a';
	const  auto expected = std::vector<char> {'a'};

	REQUIRE(to_bytes(feed) == expected);
}

TEST_CASE( "int to byte small", "[utils]" )
{
	const  int feed = 23;
	const  auto expected = std::vector<char> { 23, 0, 0, 0};

	REQUIRE(to_bytes(feed) == expected);
}

TEST_CASE( "int to byte big", "[utils]" )
{
	const  int feed = 412424;
	const  auto expected = std::vector<char> { 8, 75, 6, 0 };

	REQUIRE(to_bytes(feed) == expected);
}

TEST_CASE( "int from byte big", "[utils]" )
{
	const auto feed = std::vector<char> { 8, 75, 6, 0 };
	const auto expected = 412424;

	int result;
	from_bytes(result, feed.begin());

	REQUIRE(result == expected);
}


TEST_CASE( "join vectors", "[utils]" )
{
	auto target = std::vector<char> { 8, 75, 6, 0 };
	const auto source = std::vector<char> { 8, 75, 6, 0 };

	joinVectors(target, source);

	const auto expected = std::vector<char> { 8, 75, 6, 0, 8, 75, 6, 0 };

	REQUIRE(target == expected);
}

TEST_CASE( "construct message from payload", "[message]" )
{
	Message::SetLamportClockSize(3);

	Message message = Message({1,2,3}, {'1','2','3','4','5','6','7', '8'});

	std::vector<char> expected = {
		1, 0, 0, 0,
		2, 0, 0, 0,
		3, 0, 0, 0,

		'1', '2', '3', '4',
		'5', '6', '7', '8'
		};

	std::vector<char> result = message.getPayload();
	REQUIRE(result == expected);
}

TEST_CASE( "message to payload", "[message]" )
{
	Message::SetLamportClockSize(3);

	std::vector<char> feed = {
		1, 0, 0, 0,
		2, 0, 0, 0,
		3, 0, 0, 0,

		'1', '2', '3', '4',
		'5', '6', '7', '8'
	};

	auto lamportClock = std::vector<int> {1,2,3};
	auto agentPayload = std::vector<char> {'1','2','3','4','5','6','7', '8'};

	Message message = Message(feed);

	REQUIRE(message.getAgentMessageBody() == agentPayload);
	REQUIRE(message.getLamportClock() == lamportClock);
}

TEST_CASE( "get type from payload", "[agent message]" )
{
	const auto agentPayload = std::vector<char> {43,'2','3','4','5','6','7', '8'};

	REQUIRE(AgentMessage::getTypeFromPayload(agentPayload) == 43);
}

TEST_CASE( "participation message from payload", "[agent message]" )
{
	const auto agentPayload = std::vector<char> {
			1,
			50, 0, 0 ,64,
			3, 0, 0, 0
	};

	auto participationMessage = ParticipationMessage(agentPayload);

	REQUIRE(participationMessage.getType() == ParticipationMessage::TypeId);
	REQUIRE(participationMessage.getCandidatesCount() == 3);
	REQUIRE(participationMessage.getManagerId() == 1073741874);
}

TEST_CASE( "payload from participation message", "[agent message]" )
{
	auto participationMessage = ParticipationMessage(1073741874, 3);

	const auto expected = std::vector<char> {
			1,
			50, 0, 0 ,(char)64,
			3, 0, 0, 0
	};

	REQUIRE(participationMessage.getPayload() == expected);
}

TEST_CASE( "agent read for contest to payload", "[agent message]" )
{
	auto agentReadyForContestMessage = AgentReadyToContestMessage();

	const auto expected = std::vector<char> {
			2
	};

	REQUIRE(agentReadyForContestMessage.getPayload() == expected);
}

TEST_CASE( "agent read for contest from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			2
	};

	auto agentReadyForContestMessage = AgentReadyToContestMessage(feed);

	REQUIRE(agentReadyForContestMessage.getType() == 2);
}


TEST_CASE( "Wrap and unwrap participation", "[wrapping]" )
{
	Message::SetLamportClockSize(3);
	auto lamportClock = std::vector<int> {1,2,3};

	auto participationMessage = ParticipationMessage(0, 3);

	auto wrappedParticipation = Message(lamportClock, participationMessage.getPayload());

	auto receivedMessage = Message(wrappedParticipation.getPayload());

	REQUIRE(participationMessage.getPayload() == receivedMessage.getAgentMessageBody());
	REQUIRE(AgentMessage::getTypeFromPayload(receivedMessage.getAgentMessageBody()) == 1);
}