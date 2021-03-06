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

TEST_CASE( "participation message from payload multpile", "[agent message]" )
{
	for (int i = 0; i < 100; ++i)
	{
		auto agentPayload = std::vector<char> {
				1,
				50, 0, 0, 64
		};

		joinVectors(agentPayload, to_bytes(i));

		auto participationMessage = ParticipationMessage(agentPayload);

		REQUIRE(participationMessage.getType() == ParticipationMessage::TypeId);
		REQUIRE(participationMessage.getCandidatesCount() == i);
		REQUIRE(participationMessage.getManagerId() == 1073741874);
	}
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

TEST_CASE( "agent ready for contest to payload", "[agent message]" )
{
	auto agentReadyForContestMessage = AgentReadyToContestMessage();

	const auto expected = std::vector<char> {
			2
	};

	REQUIRE(agentReadyForContestMessage.getPayload() == expected);
}

TEST_CASE( "agent ready for contest from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			2
	};

	auto agentReadyForContestMessage = AgentReadyToContestMessage(feed);

	REQUIRE(agentReadyForContestMessage.getType() == 2);
}

TEST_CASE( "send candidate to doctor to payload", "[agent message]" )
{
	auto sendCandidateToDoctor = SendToDoctorMessage(1, 2);

	const auto expected = std::vector<char> {
			3,
			1, 0, 0, 0,
			2, 0, 0, 0
	};

	REQUIRE(sendCandidateToDoctor.getPayload() == expected);
}

TEST_CASE( "send candidate to doctor from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			3,
			1, 0, 0, 0,
			2, 0, 0, 0
	};

	auto sendCandidateToDoctor = SendToDoctorMessage(feed);

	REQUIRE(sendCandidateToDoctor.getType() == 3);
	REQUIRE(sendCandidateToDoctor.getDoctorId() == 2);
	REQUIRE(sendCandidateToDoctor.getManagerId() == 1);
}

TEST_CASE( "send candidate to docotr from payload full zero feed", "[agent message]" )
{
	auto sendCandidateToDoctor = SendToDoctorMessage(0, 0);

	const auto feed = std::vector<char> {
			3,
			0, 0, 0, 0,
			0, 0, 0, 0
	};

	REQUIRE(sendCandidateToDoctor.getPayload() == feed);

	auto sendCandidateToDoctor2 = SendToDoctorMessage(sendCandidateToDoctor.getPayload());

	REQUIRE(sendCandidateToDoctor.getType() == 3);
	REQUIRE(sendCandidateToDoctor.getDoctorId() == 0);
	REQUIRE(sendCandidateToDoctor.getManagerId() == 0);
}


TEST_CASE( "reserve saloon to payload", "[agent message]" )
{
	auto reserveSaloon = ReserveSaloon(1);

	const auto expected = std::vector<char> {
			4,
			1, 0, 0, 0,
	};

	REQUIRE(reserveSaloon.getPayload() == expected);
}

TEST_CASE( "reserve saloon  from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			4,
			1, 0, 0, 0,
	};

	auto reserveSaloon = ReserveSaloon(feed);

	REQUIRE(reserveSaloon.getType() == 4);
	REQUIRE(reserveSaloon.getManagerId() == 1);
}

TEST_CASE( "free saloon to payload", "[agent message]" )
{
	auto freeSaloon = FreeSaloon(1);

	const auto expected = std::vector<char> {
			5,
			1, 0, 0, 0
	};

	REQUIRE(freeSaloon.getPayload() == expected);
}

TEST_CASE( "free saloon  from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			5,
			1, 0, 0, 0
	};

	auto freeSaloon = FreeSaloon(feed);

	REQUIRE(freeSaloon.getType() == 5);
	REQUIRE(freeSaloon.getManagerId() == 1);
}

TEST_CASE( "pass me request to payload", "[agent message]" )
{
	auto passMeRequest = PassMeRequest(1);

	const auto expected = std::vector<char> {
			6,
			1, 0, 0, 0
	};

	REQUIRE(passMeRequest.getPayload() == expected);
}

TEST_CASE( "pass me request from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			6,
			1, 0, 0, 0
	};

	auto passMeRequest = PassMeRequest(feed);

	REQUIRE(passMeRequest.getType() == 6);
	REQUIRE(passMeRequest.getManagerId() == 1);
}

TEST_CASE( "pass me decision to payload", "[agent message]" )
{
	auto passMeDecision = PassMeDecision(2);

	const auto expected = std::vector<char> {
			7,
			2, 0, 0, 0
	};

	REQUIRE(passMeDecision.getPayload() == expected);
}

TEST_CASE( "pass me decision from payload", "[agent message]" )
{
	const auto feed = std::vector<char> {
			7,
			2, 0, 0, 0
	};

	auto passMeDecision = PassMeDecision(feed);

	REQUIRE(passMeDecision.getType() == 7);
	REQUIRE(passMeDecision.getDecision() == 2);
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