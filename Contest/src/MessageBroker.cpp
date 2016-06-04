//
// Created by lucas on 04.06.16.
//

#include "MessageBroker.hpp"

#include <chrono>

std::vector<int> MessageBroker::_targetAll = std::vector<int>();

MessageBroker::MessageBroker(const int id, const int agentsCount)
	: _id(id), _actualLamportClock(LamportClock(agentsCount))
{
	for (int i = 0; i < agentsCount; ++i)
		if (i != id)
			_targetAll.push_back(i);
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
	_running = true;
	MPI_Status status;

	while (_running)
	{
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		if (!_running)
			return;

		int messageSize;
		MPI_Get_count(&status, MPI_INT, &messageSize);

		Payload buffer(messageSize);
		MPI_Recv(&buffer[0], messageSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		auto receivedMessage = Message(buffer);

		updateLamportClock(receivedMessage.getLamportClock());

		{
			auto messageType = AgentMessage::getTypeFromPayload(receivedMessage.getAgentMessageBody());
			std::unique_lock<std::mutex> lk(messagesPool[messageType].queryLock);
			messagesPool[messageType].query.push_back(resolveMessage(messageType, receivedMessage.getAgentMessageBody()));
		}
	}
}

void MessageBroker::stop()
{
	_running = false;
}

MessageBroker::~MessageBroker()
{
	// poke our pullMessages thread
	_running = false;
	MPI_Send((const void *) (char)1, 1, MPI_BYTE, _id, 0, MPI_COMM_WORLD);
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

















