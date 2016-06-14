#include "MessageBroker.hpp"

#include <chrono>
#include <thread>
#include <utils.h>
#include <stdexcept>
#include <iostream>

std::vector<int> MessageBroker::_targetAll = std::vector<int>();
bool MessageBroker::lamportLoggingEnabled = false;
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

	messagesPool->insert({ 1, MessageSource() });
	messagesPool->insert({ 2, MessageSource() });
	messagesPool->insert({ 3, MessageSource() });
	messagesPool->insert({ 4, MessageSource() });
	messagesPool->insert({ 5, MessageSource() });
	messagesPool->insert({ 6, MessageSource() });
	messagesPool->insert({ 7, MessageSource() });
}

Message MessageBroker::wrapWithMessage(AgentMessage &agentMessage)
{
	lamportClock->update();
	return Message(lamportClock->getCurrent(), agentMessage.getPayload());
}

void MessageBroker::send(AgentMessage &agentMessage, const std::vector<int> &targets)
{
	auto message = wrapWithMessage(agentMessage);

	for (const int target : targets)
	{
		{
			std::unique_lock<std::mutex> ll(_mpiMutex);
			MPI_Send(&message.getPayload()[0], message.getPayload().size(), MPI_BYTE, target, _contestRevision,
					 MPI_COMM_WORLD);
		}
	}

	log("Send message of type " + std::to_string(agentMessage.getType()));
}

AgentMessagePtr resolveMessage(const int type, const Payload& payload, int id)
{
	switch (type)
	{
		case ParticipationMessage::TypeId:
			return std::make_shared<ParticipationMessage>(payload);

		case AgentReadyToContestMessage::TypeId:
			return std::make_shared<AgentReadyToContestMessage>(payload);

		case SendToDoctorMessage::TypeId:
			return std::make_shared<SendToDoctorMessage>(payload);

		case ReserveSaloon::TypeId:
			return std::make_shared<ReserveSaloon>(payload);

		case FreeSaloon::TypeId:
			return std::make_shared<FreeSaloon>(payload);

		case PassMeDecision::TypeId:
			return std::make_shared<PassMeDecision>(payload);

		case PassMeRequest::TypeId:
			return std::make_shared<PassMeRequest>(payload);
	}

	printVector(payload);

	throw std::runtime_error(std::to_string(type));
}

void MessageBroker::pullMessages()
{
	MPI_Status status;

	log("Receiving messages");
	int isReady;

	while (*_running)
	{
		{
			std::unique_lock<std::mutex> ll(_mpiMutex);
			MPI_Iprobe(MPI_ANY_SOURCE, _contestRevision, MPI_COMM_WORLD, &isReady, &status);
		}

		if (!isReady)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}


		Payload buffer;

		{
			int messageSize;
			std::unique_lock<std::mutex> ll(_mpiMutex);

			MPI_Get_count(&status, MPI_BYTE, &messageSize);
			buffer.resize(messageSize);
			MPI_Recv(&buffer[0], messageSize, MPI_BYTE, MPI_ANY_SOURCE, _contestRevision, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		auto receivedMessage = Message(buffer);

		lamportClock->update(receivedMessage.getLamportClock());

		auto messageType = AgentMessage::getTypeFromPayload(receivedMessage.getAgentMessageBody());
		query(resolveMessage(messageType, receivedMessage.getAgentMessageBody(), _id));

		log("Message received with type " + std::to_string(receivedMessage.getAgentMessageBody()[0]));
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
	std::unique_lock<std::mutex> ll((*messagesPool)[agentMessage->getType()].queryLock);
	(*messagesPool)[agentMessage->getType()].query.push_back(agentMessage);
}

void MessageBroker::retract(AgentMessagePtr agentMessage)
{
	query(agentMessage);
}

void MessageBroker::log(const std::string &message)
{
	std::ostringstream stringStream;
	stringStream << "[Broker " << _id << "] ";

	stringStream << message;

	if (lamportLoggingEnabled)
		stringStream << *lamportClock;

	printf("%s\n", stringStream.str().c_str());
}

void MessageBroker::setLogLamport(const bool log)
{
	MessageBroker::lamportLoggingEnabled = log;
}
























