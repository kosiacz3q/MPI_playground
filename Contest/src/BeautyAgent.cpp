//
// Created by lucas on 04.06.16.
//

#include "BeautyAgent.hpp"
#include "MessageBroker.hpp"
#include "Messages.hpp"

#include <cstdio>
#include <ctime>
#include <cstdlib>


int BeautyAgent::MaxCandidatesCount = 10;

BeautyAgent::BeautyAgent(const int id, const int managersCount)
	:_id(id), _broker(id, managersCount), _managersCount(managersCount)
{
	_managersCandidatesCount = new int[managersCount];

	_puller= std::thread(&MessageBroker::pullMessages, &_broker);

	printf("[Agent %i] ready for action\n", id);
}

void BeautyAgent::run()
{
	prepare();

	waitForAllManagersToBeReady();

	_broker.stop();
	_puller.join();
}

BeautyAgent::~BeautyAgent()
{
	delete [] _managersCandidatesCount;
	printf("[Agent %i] is melting\n", _id);
}

void BeautyAgent::prepare()
{
	std::srand(std::time(0));

	_managersCandidatesCount[_id] = std::rand() % MaxCandidatesCount;

	auto participationMessage = ParticipationMessage(_id, _managersCandidatesCount[_id]);
	_broker.send(participationMessage);
}

void BeautyAgent::waitForAllManagersToBeReady()
{
	int i = _managersCount;

	auto readyMessage = AgentReadyToContestMessage();
	_broker.send(readyMessage);

	while (--i)
	{
		_broker.receive<AgentReadyToContestMessage>();
	}
}







