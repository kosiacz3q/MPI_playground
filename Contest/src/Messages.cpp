//
// Created by lucas on 04.06.16.
//

#include "Messages.hpp"

#include <utils.h>

size_t Message::PlainMessagePayloadOffset = 0;

void Message::SetLamportClockSize(const uint32_t n)
{
	PlainMessagePayloadOffset = sizeof(int) * n;
}

Message::Message(const Payload& payload)
		:_payload(payload),
		 _lamportClock(LamportClock()),
		 _agentPayload(Payload())
{
	for (auto lamportIter = _payload.begin(); lamportIter < _payload.begin() + PlainMessagePayloadOffset; lamportIter += 4)
	{
		int agentClock;
		from_bytes(agentClock, lamportIter);
		_lamportClock.push_back(agentClock);
	}

	std::copy(_payload.begin() + PlainMessagePayloadOffset, _payload.end(), std::back_inserter(_agentPayload));
}

Message::Message(const LamportClock& lamportClock, const Payload& agentMessagePayload)
	: _payload(Payload())
{
	for (const int &clockValue : lamportClock)
		joinVectors(_payload, to_bytes(clockValue));

	std::copy(agentMessagePayload.begin(), agentMessagePayload.end(), std::back_inserter(_payload));
}

ParticipationMessage::ParticipationMessage(Payload payload)
	: AgentMessage(1, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
	from_bytes(_candidatesCount, _payload.begin() + 5);
}

ParticipationMessage::ParticipationMessage(int managerId, int candidatesCount)
	: AgentMessage(1)
{
	joinVectors(_payload, to_bytes(managerId));
	joinVectors(_payload, to_bytes(candidatesCount));
}


AgentReadyToContestMessage::AgentReadyToContestMessage(Payload payload)
	: AgentMessage(2, payload)
{

}

AgentReadyToContestMessage::AgentReadyToContestMessage()
	: AgentMessage(2)
{

}



