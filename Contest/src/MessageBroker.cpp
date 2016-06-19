#include "MessageBroker.hpp"
#include "Logger.hpp"

#include <thread>
#include <utils.h>
#include <iostream>

std::vector<int> MessageBroker::_targetAll = std::vector<int>();
LamportVectorClockPtr MessageBroker::lamportClock = nullptr;

MessageBroker::MessageBroker(const int id, const int agentsCount, const int contestRevision)
	: _id(id),
	  _running(new bool),
	  _contestRevision(contestRevision)
{
	if (_targetAll.empty())
	{
		for (int i = 0; i < agentsCount; ++i)
			if (i != id)
				_targetAll.push_back(i);
	}

	*_running = true;

	messagesPool = new std::map<int, MessageSource>();

	registerMessageType<ParticipationMessage>();
	registerMessageType<AgentReadyToContestMessage>();
	registerMessageType<SendToDoctorMessage>();
	registerMessageType<DoctorsVisitEnds>();
	registerMessageType<ReserveSaloon>();
	registerMessageType<FreeSaloon>();
	registerMessageType<PassMeRequest>();
	registerMessageType<PassMeDecision>();
}

Message MessageBroker::wrapWithMessage(AgentMessage &agentMessage)
{
	return Message(lamportClock->getCurrent(), agentMessage.getPayload());
}

void MessageBroker::send(AgentMessage &agentMessage, const std::vector<int> &targets)
{
	if (targets.empty())
		return;

	lamportClock->update();

	auto message = wrapWithMessage(agentMessage);

	for (const int target : targets)
	{
		{
			std::unique_lock<std::mutex> ll(_mpiMutex);
			MPI_Send(&message.getPayload()[0], message.getPayload().size(), MPI_BYTE, target, _contestRevision,
					 MPI_COMM_WORLD);
		}
	}

	log("Message of type " + std::to_string(agentMessage.getType()) + " sent");
}

AgentMessagePtr MessageBroker::resolveMessage(const int type, const Payload& payload)
{
	auto handler = (*messagesPool).find(type);

	if (handler == (*messagesPool).end())
	{
		log("Invalid message type " + std::to_string(type));
	}

	return handler->second.unpack(payload);
}

void MessageBroker::pullMessages()
{
	MPI_Status status;

	log("Receiving messages");
	int isReady;

	while (*_running)
	{
		Payload buffer;

		{
			std::unique_lock<std::mutex> ll(_mpiMutex);
			MPI_Iprobe(MPI_ANY_SOURCE, _contestRevision, MPI_COMM_WORLD, &isReady, &status);

			if (isReady)
			{
				int messageSize;
				MPI_Get_count(&status, MPI_BYTE, &messageSize);
				buffer.resize(messageSize);

				MPI_Recv(&buffer[0], messageSize, MPI_BYTE, MPI_ANY_SOURCE, _contestRevision, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		}

		if (!isReady)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		auto receivedMessage = Message(buffer);

		lamportClock->update(receivedMessage.getLamportClock());

		auto messageType = AgentMessage::getTypeFromPayload(receivedMessage.getAgentMessageBody());

		auto unpacked = resolveMessage(messageType, receivedMessage.getAgentMessageBody());
		query(unpacked);

		log("Message received with type " + std::to_string(unpacked->getType()));
	}

	log("End of receiving messages");
}

void MessageBroker::stop()
{
	*_running = false;
}

MessageBroker::~MessageBroker()
{
	delete messagesPool;
	printf("[Broker %i] destroyed\n", _id);
}

void MessageBroker::query(AgentMessagePtr agentMessage)
{
	auto handler = (*messagesPool).find(agentMessage->getType())->second;

	std::unique_lock<std::mutex> ll(handler.queryLock);
	log("Quering type " + std::to_string(agentMessage->getType()));
	handler.query->push_back(agentMessage);
}

void MessageBroker::retract(AgentMessagePtr agentMessage)
{
	query(agentMessage);
}

void MessageBroker::log(const std::string &message)
{
	static std::string whoami = "Broker " + std::to_string(_id) + "";

	Logger::log(whoami, message);
}
























