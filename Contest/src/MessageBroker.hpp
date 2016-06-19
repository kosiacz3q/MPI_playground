#ifndef CONTEST_MESSAGEBROKER_HPP
#define CONTEST_MESSAGEBROKER_HPP

#include <mpi.h>
#include <map>
#include <list>
#include <condition_variable>

#include "Messages.hpp"
#include "LamportVectorClock.hpp"

class MessageBroker
{
public:

	MessageBroker(const int id, const int agentsCount, const int contestRevision);

	virtual ~MessageBroker();

	void send(AgentMessage& agentMessage, const std::vector<int> &targets = _targetAll);

	template<typename T>
	AgentMessagePtr receive()
	{
		const int typeId = T::getTypeId();
		auto source = (*messagesPool).find(typeId)->second;

		log("Waiting for " + std::to_string(typeId));

		while (true)
		{
			{
				std::unique_lock<std::mutex> lk(source.queryLock);

				if (!source.query->empty())
				{
					AgentMessagePtr message = source.query->front();
					source.query->pop_front();

					return message;
				}
			}

			{
				std::unique_lock<std::mutex> lk(source.notifierLock);
				source.notifier.wait_for(lk, std::chrono::milliseconds(100));
			}
		}
	}

	template<typename T>
	bool isAvailable()
	{
		auto source = (*messagesPool).find(T::getTypeId())->second;

		//log("Availability check for " + std::to_string(T::getTypeId()) + " " + std::to_string(source.query->size()));

		return !source.query->empty();
	}

	void retract(AgentMessagePtr agentMessage);

	void pullMessages();

	void stop();

	static LamportVectorClockPtr lamportClock;

private:

	template<typename T>
	void registerMessageType()
	{
		messagesPool->insert({
			T::TypeId
			, MessageSource([](const Payload payload) {
					return (AgentMessagePtr)std::make_shared<T>(payload);})
		});
	}

	void log(const std::string& message);

	void query(AgentMessagePtr agentMessage);

	Message wrapWithMessage(AgentMessage& agentMessage);

	AgentMessagePtr resolveMessage(const int type, const Payload& payload);

	class MessageSource
	{
	public:

		MessageSource(std::function<AgentMessagePtr(const Payload&)> unpacker) :
			query (std::make_shared<std::list<AgentMessagePtr>>()), unpack(unpacker)
		{

		}

		MessageSource(const MessageSource& source) :
			query (source.query), unpack(source.unpack)
		{

		}

		std::condition_variable notifier;
		std::mutex notifierLock;
		std::shared_ptr<std::list<AgentMessagePtr>> query;
		std::mutex queryLock;
		std::function<AgentMessagePtr(const Payload&)> unpack;
	};

	static std::vector<int> _targetAll;

	int _id;
	std::map<int, MessageSource> *messagesPool;
	bool* _running;
	std::mutex _mpiMutex;
	int _contestRevision;
};


#endif //CONTEST_MESSAGEBROKER_HPP
