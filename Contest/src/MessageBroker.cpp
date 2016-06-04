//
// Created by lucas on 04.06.16.
//

#include "MessageBroker.hpp"

#include <chrono>
#include <thread>

std::vector<int> MessageBroker::_targetAll = std::vector<int>();

MessageBroker::MessageBroker(const int id, const int agentsCount)
	: _id(id), _actualLamportClock(LamportClock(agentsCount)), _running(new bool)
{
	for (int i = 0; i < agentsCount; ++i)
		if (i != id)
			_targetAll.push_back(i);

	*_running = true;
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

	for (const int target : targets)
	{
		MPI_Send( &message.getPayload()[0], message.getPayload().size(), MPI_BYTE, target, agentMessage.getType(), MPI_COMM_WORLD);
	}
}

template<typename T>
auto MessageBroker::receive() -> T
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> lk(messagesPool[T::TypeId].queryLock);

			if (!messagesPool[T::TypeId].query.empty())
			{
				T message = messagesPool[T::TypeId].query.back();
				messagesPool[T::TypeId].query.pop_back();
				return message;
			}
		}

		{
			std::unique_lock<std::mutex> lk(messagesPool[T::TypeId].notifierLock);
			messagesPool[T::TypeId].notifier.wait_until(
					lk,
					std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<float> (100)));
		}
	}
}

AgentMessage resolveMessage(const int type, const Payload& payload)
{
	switch (type)
	{
		case ParticipationMessage::TypeId :
			return ParticipationMessage(payload);
	}
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
			std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<float> (100)));
			//printf("No message\n");
			continue;
		}

		printf("[Broker %i] received message\n", _id);

		int messageSize;
		MPI_Get_count(&status, MPI_BYTE, &messageSize);

		printf("[Broker %i] message count %i\n", _id, messageSize);

		Payload buffer(messageSize);
		MPI_Recv(&buffer[0], messageSize, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		auto receivedMessage = Message(buffer);

		updateLamportClock(receivedMessage.getLamportClock());

		{
			auto messageType = AgentMessage::getTypeFromPayload(receivedMessage.getAgentMessageBody());
			std::unique_lock<std::mutex> lk(messagesPool[messageType].queryLock);
			messagesPool[messageType].query.push_back(resolveMessage(messageType, receivedMessage.getAgentMessageBody()));
		}
	}

	printf("[Broker %i] stops receiving messages\n", _id);
}

void MessageBroker::stop()
{
	*_running = false;
}

MessageBroker::~MessageBroker()
{
	printf("[Broker %i] destroyed\n", _id);
}

void MessageBroker::query(AgentMessage &)
{

}

void MessageBroker::updateLamportClock(const LamportClock &lamportClock)
{
	std::unique_lock<std::mutex> ll(_lamportClockMutex);

	for (int i = 0; i < lamportClock.size(); ++i)
		if (lamportClock[i] > _actualLamportClock[i])
			_actualLamportClock[i] = lamportClock[i];
}


















