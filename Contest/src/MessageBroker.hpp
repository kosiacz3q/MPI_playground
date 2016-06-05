//
// Created by lucas on 04.06.16.
//

#ifndef CONTEST_MESSAGEBROKER_HPP
#define CONTEST_MESSAGEBROKER_HPP

#include <mpi.h>
#include <map>
#include <condition_variable>

#include "Messages.hpp"

class MessageBroker
{
public:

	MessageBroker(const int id, const int agentsCount);

	virtual ~MessageBroker();

	void send(AgentMessage& agentMessage, const std::vector<int> &targets = _targetAll);

	template<typename T>
	AgentMessagePtr receive()
	{
		const int typeId = T::getTypeId();

		while (true)
		{
			{
				//printf("Waiting for lock\n");
				std::unique_lock<std::mutex> lk((*messagesPool)[typeId].queryLock);
				//printf("Lock received\n");

				if (!(*messagesPool)[typeId].query.empty())
				{
					printf("[Broker %i] There is message in the poll %i\n", _id, typeId);
					AgentMessagePtr message = (*messagesPool)[typeId].query.back();
					(*messagesPool)[typeId].query.pop_back();
					printf("[Broker %i]  Message get\n", _id);
					return message;
				}

				//printf("There is no messages in the poll\n");
			}

			{
				std::unique_lock<std::mutex> lk((*messagesPool)[typeId].notifierLock);
				//printf("Wait until\n");
				(*messagesPool)[typeId].notifier.wait_for(lk, std::chrono::milliseconds(100));
			}
		}
	}

	void pullMessages();

	void stop();

private:

	void query(AgentMessagePtr agentMessage);

	void updateLamportClock(const LamportClock& lamportClock);

	Message wrapWithMessage(AgentMessage& agentMessage);

	int _id;

	std::mutex _lamportClockMutex;
	LamportClock _actualLamportClock;

	static std::vector<int> _targetAll;

	class MessageSource
	{
	public:

		MessageSource() :
			query (std::vector<AgentMessagePtr>())
		{

		}

		MessageSource(const MessageSource& source) :
			query (source.query)
		{

		}

		std::condition_variable notifier;
		std::mutex notifierLock;
		std::vector<AgentMessagePtr> query;
		std::mutex queryLock;
	};

	std::map<int, MessageSource> *messagesPool;

	bool* _running;
};


#endif //CONTEST_MESSAGEBROKER_HPP
