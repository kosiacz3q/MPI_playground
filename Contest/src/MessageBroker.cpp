//
// Created by lucas on 04.06.16.
//

#include "MessageBroker.hpp"

#include <chrono>
#include <thread>
#include <utils.h>

std::vector<int> MessageBroker::_targetAll = std::vector<int>();

MessageBroker::MessageBroker(const int id, const int agentsCount)
	: _id(id), _actualLamportClock(LamportClock(agentsCount)), _running(new bool)
{
	for (int i = 0; i < agentsCount; ++i)
		if (i != id)
			_targetAll.push_back(i);

	*_running = true;

	messagesPool = new std::map<int, MessageSource>();

	messagesPool->insert({ 1, MessageSource() });
	messagesPool->insert({ 2, MessageSource() });
}

Message MessageBroker::wrapWithMessage(AgentMessage &agentMessage)
{
	std::unique_lock<std::mutex> ll(_lamportClockMutex);
	++_actualLamportClock[_id];
	return Message(_actualLamportClock, agentMessage.getPayload());
}

void MessageBroker::send(AgentMessage &agentMessage, const std::vector<int> &targets)
{
	auto message = wrapWithMessage(agentMessage);

	printf("[Broker %i] sending message  %i\n", _id, agentMessage.getType());

	for (const int target : targets)
	{
		MPI_Send( &message.getPayload()[0], message.getPayload().size(), MPI_BYTE, target, agentMessage.getType(), MPI_COMM_WORLD);
	}

	printf("[Broker %i] sending message  %i success\n", _id, agentMessage.getType());
}

AgentMessagePtr resolveMessage(const int type, const Payload& payload, int id)
{
	printf("[Broker %i] resolving %i\n",id, type);

	switch (type)
	{
		case ParticipationMessage::TypeId:
			return std::make_shared<ParticipationMessage>(payload);

		case AgentReadyToContestMessage::TypeId:
			return std::make_shared<AgentReadyToContestMessage>(payload);
	}

	printVector(payload);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	throw "uknown message type";
}

void MessageBroker::pullMessages()
{
	MPI_Status status;

	printf("[Broker %i] starts receiving messages\n", _id);
	int isReady;

	while (*_running)
	{

		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &isReady, &status);

		if (!isReady)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			//printf("No message\n");
			continue;
		}

		int messageSize;
		MPI_Get_count(&status, MPI_BYTE, &messageSize);

		printf("[Broker %i] received message size %i\n", _id, messageSize);

		Payload buffer(messageSize);
		MPI_Recv(&buffer[0], messageSize, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		auto receivedMessage = Message(buffer);

		updateLamportClock(receivedMessage.getLamportClock());

		auto messageType = AgentMessage::getTypeFromPayload(receivedMessage.getAgentMessageBody());
		query(resolveMessage(messageType, receivedMessage.getAgentMessageBody(), _id));

		printf("[Broker %i] queried message type %i\n", _id, receivedMessage.getAgentMessageBody()[0]);
	}

	printf("[Broker %i] stops receiving messages\n", _id);
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

	printf("[Broker %i] query mType %i\n", _id, agentMessage->getType());

	(*messagesPool)[agentMessage->getType()].query.push_back(agentMessage);

	printf("[Broker %i] query mType %i success\n", _id, agentMessage->getType());
}

void MessageBroker::updateLamportClock(const LamportClock &lamportClock)
{
	std::unique_lock<std::mutex> ll(_lamportClockMutex);

	for (int i = 0; i < lamportClock.size(); ++i)
		if (lamportClock[i] > _actualLamportClock[i])
			_actualLamportClock[i] = lamportClock[i];
}


















