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

	template <typename T>
	auto receive() -> T;

	void pullMessages();

	void stop();

	void poke();

private:

	void query(AgentMessage& agentMessage);

	void updateLamportClock(const LamportClock& lamportClock);

	Message wrapWithMessage(AgentMessage& agentMessage);

	int _id;

	std::mutex _lamportClockMutex;
	LamportClock _actualLamportClock;

	static std::vector<int> _targetAll;

	struct MessageSource
	{
		std::condition_variable notifier;
		std::mutex notifierLock;
		std::vector<AgentMessage> query;
		std::mutex queryLock;
	};

	std::map<int, MessageSource> messagesPool;

	bool* _running;
};


#endif //CONTEST_MESSAGEBROKER_HPP
