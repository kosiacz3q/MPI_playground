//
// Created by lucas on 04.06.16.
//

#ifndef CONTEST_MESSAGE_HPP
#define CONTEST_MESSAGE_HPP

#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>


typedef std::vector<char> Payload;
typedef std::vector<int> LamportClock;
typedef char MessageType;

class Message
{
public:

	static void SetLamportClockSize(const uint32_t n);

	Message(const Payload& payload);
	Message(const LamportClock& lamportClock, const Payload& agentMessagePayload);

	LamportClock& getLamportClock() { return _lamportClock; } const
	Payload& getAgentMessageBody() { return _agentPayload; } const
	Payload& getPayload() { return _payload; } const

	virtual ~Message(){}

protected:

	Payload _payload;

	LamportClock _lamportClock;
	Payload _agentPayload;

	static size_t PlainMessagePayloadOffset;

private:

};
//======================================================================================================================

class AgentMessage
{
public:

	AgentMessage(MessageType type)
		: _payload(Payload()), _type(type)
	{
		_payload.push_back(_type);
	}

	AgentMessage(MessageType type, Payload& payload)
			: _payload(payload), _type(type)
	{

	}

	virtual ~AgentMessage() {}

	MessageType getType() { return _type; } const

	Payload getPayload() { return _payload; } const

	static MessageType getTypeFromPayload(const Payload& payload) { return payload[0]; }

protected:

	Payload _payload;

private:
	MessageType _type;
};
//======================================================================================================================

typedef std::shared_ptr<AgentMessage> AgentMessagePtr;
//======================================================================================================================

class ParticipationMessage : public AgentMessage
{
public:

	ParticipationMessage(Payload payload);
	ParticipationMessage(int managerId, int candidatesCount);

	int getManagerId() { return _managerId; } const
	int getCandidatesCount() { return _candidatesCount; } const

	static constexpr int TypeId = 1;
	static int getTypeId() {return 1; }

private:

	int _managerId;
	int _candidatesCount;
};
//======================================================================================================================

class AgentReadyToContestMessage : public AgentMessage
{
public:

	AgentReadyToContestMessage(Payload payload);
	AgentReadyToContestMessage();

	static constexpr int TypeId = 2;
	static int getTypeId() {return 2; }
};
//======================================================================================================================

class SendToDoctorMessage : public AgentMessage
{
public:

	SendToDoctorMessage(Payload payload);
	SendToDoctorMessage(const int managerId, int doctorId);

	int getManagerId() { return _managerId; } const
	int getDoctorId() { return  _doctorId; } const

	static constexpr int TypeId = 3;
	static int getTypeId() {return 3; }

private:

	int _managerId;
	int _doctorId;
};
//======================================================================================================================

class ReserveSaloon : public AgentMessage
{
public:

	ReserveSaloon(Payload payload);
	ReserveSaloon(const int managerId, int spotsCount);

	int getManagerId() { return _managerId; } const
	int getSpotsCount() { return  _spotsCount; } const

	static constexpr int TypeId = 4;
	static int getTypeId() {return 4; }

private:

	int _managerId;
	int _spotsCount;
};
//======================================================================================================================

class FreeSaloon : public AgentMessage
{
public:

	FreeSaloon(Payload payload);
	FreeSaloon(const int managerId, int spotsCount);

	int getManagerId() { return _managerId; } const
	int getSpotsCount() { return  _spotsCount; } const

	static constexpr int TypeId = 5;
	static int getTypeId() {return 5; }

private:

	int _managerId;
	int _spotsCount;
};
//======================================================================================================================
#endif //CONTEST_MESSAGE_HPP
