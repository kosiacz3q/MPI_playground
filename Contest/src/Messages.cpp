//
// Created by lucas on 04.06.16.
//

#include "Messages.hpp"

#include <utils.h>

size_t Message::PlainMessagePayloadOffset = 0;

void Message::SetLamportClockSize(const int n)
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
	: AgentMessage(ParticipationMessage::TypeId, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
	from_bytes(_candidatesCount, _payload.begin() + 5);
}

ParticipationMessage::ParticipationMessage(int managerId, int candidatesCount)
	: AgentMessage(ParticipationMessage::TypeId), _managerId(managerId), _candidatesCount(candidatesCount)
{
	joinVectors(_payload, to_bytes(managerId));
	joinVectors(_payload, to_bytes(candidatesCount));
}


AgentReadyToContestMessage::AgentReadyToContestMessage(Payload payload)
	: AgentMessage(AgentReadyToContestMessage::TypeId, payload)
{

}

AgentReadyToContestMessage::AgentReadyToContestMessage()
	: AgentMessage(AgentReadyToContestMessage::TypeId)
{

}


SendToDoctorMessage::SendToDoctorMessage(Payload payload)
	: AgentMessage(SendToDoctorMessage::TypeId, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
	from_bytes(_doctorId, _payload.begin() + 5);
}

SendToDoctorMessage::SendToDoctorMessage(const int managerId, const int doctorId)
	: AgentMessage(SendToDoctorMessage::TypeId), _managerId(managerId), _doctorId(doctorId)
{
	joinVectors(_payload, to_bytes(managerId));
	joinVectors(_payload, to_bytes(doctorId));
}



DoctorsVisitEnds::DoctorsVisitEnds(Payload payload)
	: AgentMessage(DoctorsVisitEnds::TypeId, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
	from_bytes(_doctorId, _payload.begin() + 5);
}


DoctorsVisitEnds::DoctorsVisitEnds(const int managerId, int doctorId)
	: AgentMessage(DoctorsVisitEnds::TypeId), _managerId(managerId), _doctorId(doctorId)
{
	joinVectors(_payload, to_bytes(managerId));
	joinVectors(_payload, to_bytes(doctorId));
}

ReserveSaloon::ReserveSaloon(Payload payload)
		: AgentMessage(ReserveSaloon::TypeId, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
}


ReserveSaloon::ReserveSaloon(const int managerId)
		: AgentMessage(ReserveSaloon::TypeId), _managerId(managerId)
{
	joinVectors(_payload, to_bytes(managerId));
}

FreeSaloon::FreeSaloon(Payload payload)
	: AgentMessage(FreeSaloon::TypeId, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
}


FreeSaloon::FreeSaloon(const int managerId)
	: AgentMessage(FreeSaloon::TypeId), _managerId(managerId)
{
	joinVectors(_payload, to_bytes(managerId));
}

PassMeRequest::PassMeRequest(Payload payload)
		: AgentMessage(PassMeRequest::TypeId, payload)
{
	from_bytes(_managerId, _payload.begin() + 1);
}


PassMeRequest::PassMeRequest(const int managerId)
		: AgentMessage(PassMeRequest::TypeId), _managerId(managerId)
{
	joinVectors(_payload, to_bytes(managerId));
}

PassMeDecision::PassMeDecision(Payload payload)
		: AgentMessage(PassMeDecision::TypeId, payload)
{
	from_bytes(_decision, _payload.begin() + 1);
}


PassMeDecision::PassMeDecision(const int decision)
		: AgentMessage(PassMeDecision::TypeId), _decision(decision)
{
	joinVectors(_payload, to_bytes(decision));
}



