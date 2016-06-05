//
// Created by lucas on 04.06.16.
//

#include "BeautyAgent.hpp"
#include "MessageBroker.hpp"
#include "Messages.hpp"


#include <utils.h>
#include <limits>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <algorithm>


int BeautyAgent::MaxCandidatesCount = 3;

BeautyAgent::BeautyAgent(const int id, const int managersCount, const int doctorsCount, const int saloonCapacity)
	:_id(id),
	 _broker(id, managersCount),
	 _managersCount(managersCount),
	 _doctorsCount(doctorsCount),
	 _minInDoctorQueue(0),
	 _saloonCapacity(saloonCapacity)
{
	_managersCandidatesCount = new int[managersCount];
	_queueToSaloon = new int[managersCount];

	for (int i = 0; i < managersCount; ++i)
		_queueToSaloon[i] = std::numeric_limits<int>::max();

	_puller= std::thread(&MessageBroker::pullMessages, &_broker);

	printf("[Agent %i] ready for action\n", id);
}

void BeautyAgent::run()
{
	prepare();

	checkInDoctor();

	printf("[Agent %i] saloon ticket is %i\n", _id, _queueToSaloon[_id]);

	waitForAllManagersToBeReady();

	_broker.stop();
	_puller.join();
}

BeautyAgent::~BeautyAgent()
{
	delete [] _managersCandidatesCount;
	delete [] _queueToSaloon;

	printf("[Agent %i] is melting\n", _id);
}

void BeautyAgent::prepare()
{
	std::srand((unsigned int) (std::time(0) + _id));

	_managersCandidatesCount[_id] = std::rand() % MaxCandidatesCount + 1;

	printf("[Agent %i] choose to start with %i candidates\n", _id, _managersCandidatesCount[_id]);

	auto participationMessage = ParticipationMessage(_id, _managersCandidatesCount[_id]);
	_broker.send(participationMessage);

	int i = _managersCount;

	while (--i)
	{
		auto agentData = std::dynamic_pointer_cast<ParticipationMessage>(_broker.receive<ParticipationMessage>());
		_managersCandidatesCount[agentData->getManagerId()] = agentData->getCandidatesCount();
	}

	printVector("[Agent " + std::to_string(_id) + "] candidates count",_managersCandidatesCount, _managersCandidatesCount + _managersCount - 1);
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

void BeautyAgent::checkInDoctor()
{
	int turn = 0;

	int doctorsQueue[_doctorsCount];
	std::vector<int> targets;

	while(_managersCandidatesCount[_id])
	{
		//clear queue
		for (int i = 0; i < _doctorsCount; ++i)
			doctorsQueue[i] = std::numeric_limits<int>::max();

		// query other managers who have non checked candidates
		targets.clear();
		for (int i = 0; i < _managersCount; ++i)
			if (_managersCandidatesCount[i] > 0 && i != _id)
				targets.push_back(i);

		// send message with choosen doctor
		int choosenDoctor = std::rand() % _doctorsCount;
		auto sendToDoctorMessage = SendToDoctorMessage(_id, choosenDoctor);
		doctorsQueue[choosenDoctor] = _id;

		_broker.send(sendToDoctorMessage, targets);

		// receive chooses of others
		int expectedResponsesCount = (int) targets.size();

		printVector("[Agent " + std::to_string(_id) + "] targets turn [" + std::to_string(turn) + "]",&targets[0], &(targets[targets.size() - 1]));

		while(expectedResponsesCount--)
		{
			while (!targets.empty())
			{
				auto response = std::dynamic_pointer_cast<SendToDoctorMessage>(_broker.receive<SendToDoctorMessage>());

				printf("[Agent %i] received %i from %i turn [%i]\n", _id, response->getDoctorId(),
					   response->getManagerId(), turn);

				if (std::find(targets.begin(), targets.end(), response->getManagerId()) ==  targets.end())
				{
					_broker.retract(response);
					continue;
				}

				if (doctorsQueue[response->getDoctorId()] > response->getManagerId())
					doctorsQueue[response->getDoctorId()] = response->getManagerId();

				targets.erase(std::find(targets.begin(), targets.end(), response->getManagerId()));
			}
		}

		// perform doctor check
		for (const int managerInDoctor : doctorsQueue)
			if (managerInDoctor != std::numeric_limits<int>::max())
				if (--_managersCandidatesCount[managerInDoctor] == 0)
				{
					//TODO add some sleep
					_queueToSaloon[managerInDoctor] = _minInDoctorQueue++;
				}

		printVector("[Agent " + std::to_string(_id) + "] current state turn [" + std::to_string(turn++) + "]", _managersCandidatesCount, _managersCandidatesCount + _managersCount - 1);
	}
}
