#ifndef CONTEST_MESSAGEBROKER_HPP
#define CONTEST_MESSAGEBROKER_HPP

#include <mpi.h>
#include <map>
#include <list>
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
				std::unique_lock<std::mutex> lk((*messagesPool)[typeId].queryLock);

				if (!(*messagesPool)[typeId].query.empty())
				{
					AgentMessagePtr message = (*messagesPool)[typeId].query.front();
					(*messagesPool)[typeId].query.pop_front();

					return message;
				}
			}

			{
				std::unique_lock<std::mutex> lk((*messagesPool)[typeId].notifierLock);
				//printf("Wait until\n");
				(*messagesPool)[typeId].notifier.wait_for(lk, std::chrono::milliseconds(100));
			}
		}
	}

	template<typename T>
	bool isAvailable()
	{
		return !(*messagesPool)[T::getTypeId()].query.empty();
	}

	void retract(AgentMessagePtr agentMessage);

	void pullMessages();

	void stop();

	static void setLogLamport(const bool log);

private:

	void log(const std::string& message);

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
			query (std::list<AgentMessagePtr>())
		{

		}

		MessageSource(const MessageSource& source) :
			query (source.query)
		{

		}

		std::condition_variable notifier;
		std::mutex notifierLock;
		std::list<AgentMessagePtr> query;
		std::mutex queryLock;
	};

	std::map<int, MessageSource> *messagesPool;

	bool* _running;
	static bool lamportLoggingEnabled;

	std::mutex _mpiMutex;
};


#endif //CONTEST_MESSAGEBROKER_HPP
