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

BeautyAgent::BeautyAgent(const int id, const int managersCount, const int doctorsCount, const int saloonCapacity, const int contestRevision)
	:_id(id),
	 _managersCount(managersCount),
	 _doctorsCount(doctorsCount),
	 _minInDoctorQueue(0),
	 _saloonCapacity(saloonCapacity),
	 _running(true)
{
	_broker = std::make_shared<MessageBroker>(id, managersCount, contestRevision);
	_managersCandidatesCount = new int[managersCount];
	_queueToSaloon = new int[managersCount];

	for (int i = 0; i < managersCount; ++i)
		_queueToSaloon[i] = std::numeric_limits<int>::max();

	_doctorComplete = std::make_shared<PassingState>(PassingState::REJECT);

	_puller = std::thread(&MessageBroker::pullMessages, &*_broker);
	_passer = std::thread(&BeautyAgent::passerLoop, this);

	printf("[Agent %i] ready for action\n", id);
}

void BeautyAgent::run()
{
	prepare();

	if (_candidatesCount != 0)
	{
		checkInDoctor();

		printf("[Agent %i] saloon ticket for saloon [%i]\n", _id, _queueToSaloon[_id]);

		waitInDoctor();

		checkInSaloon();
	}
	else
	{
		printf("[Agent %i] is not participating in current contest\n", _id);
	}

	waitForAllManagersToBeReady();

	_running = false;
	_broker->stop();
	_puller.join();
	_passer.join();
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

	_managersCandidatesCount[_id] = _candidatesCount = std::rand() % MaxCandidatesCount + 1;

	printf("[Agent %i] choose to start with %i candidates\n", _id, _managersCandidatesCount[_id]);

	auto participationMessage = ParticipationMessage(_id, _managersCandidatesCount[_id]);
	_broker->send(participationMessage);

	int i = _managersCount;

	while (--i)
	{
		auto agentData = std::dynamic_pointer_cast<ParticipationMessage>(_broker->receive<ParticipationMessage>());
		_managersCandidatesCount[agentData->getManagerId()] = agentData->getCandidatesCount();
	}

	printVector("[Agent " + std::to_string(_id) + "] candidates count",_managersCandidatesCount, _managersCandidatesCount + _managersCount - 1);
}

void BeautyAgent::waitForAllManagersToBeReady()
{
	int i = _managersCount - 1;

	auto readyMessage = AgentReadyToContestMessage();
	_broker->send(readyMessage);

	printf("[Agent %i] Waiting for others\n", _id);

	while (i)
	{
		if (_broker->isAvailable<AgentReadyToContestMessage>())
		{
			--i;
			_broker->receive<AgentReadyToContestMessage>();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	printf("[Agent %i] End of waiting for others\n", _id);
}

void BeautyAgent::checkInDoctor()
{
	int turn = 0;

	int doctorsQueue[_doctorsCount];
	std::vector<int> targets;

	int managerCandidatesPendingToDoctor[_managersCount];
	memcpy(managerCandidatesPendingToDoctor, _managersCandidatesCount, _managersCount * sizeof(int));

	while(managerCandidatesPendingToDoctor[_id])
	{
		//clear queue
		for (int i = 0; i < _doctorsCount; ++i)
			doctorsQueue[i] = std::numeric_limits<int>::max();

		// query other managers who have non checked candidates
		targets.clear();
		for (int i = 0; i < _managersCount; ++i)
			if (managerCandidatesPendingToDoctor[i] > 0 && i != _id)
				targets.push_back(i);

		// send message with choosen doctor
		int choosenDoctor = std::rand() % _doctorsCount;
		auto sendToDoctorMessage = SendToDoctorMessage(_id, choosenDoctor);
		doctorsQueue[choosenDoctor] = _id;

		_broker->send(sendToDoctorMessage, targets);

		// receive chooses of others
		int expectedResponsesCount = (int) targets.size();

		printVector("[Agent " + std::to_string(_id) + "] targets turn [" + std::to_string(turn) + "]",&targets[0], &(targets[targets.size() - 1]));

		while(expectedResponsesCount-- > 0)
		{
			while (!targets.empty())
			{
				auto response = std::dynamic_pointer_cast<SendToDoctorMessage>(_broker->receive<SendToDoctorMessage>());

				printf("[Agent %i] received %i from %i turn [%i]\n", _id, response->getDoctorId(),
					   response->getManagerId(), turn);

				if (std::find(targets.begin(), targets.end(), response->getManagerId()) ==  targets.end())
				{
					_broker->retract(response);
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
				if (--managerCandidatesPendingToDoctor[managerInDoctor] == 0)
				{
					_queueToSaloon[managerInDoctor] = _minInDoctorQueue++;
				}

		printVector("[Agent " + std::to_string(_id) + "] current state turn [" + std::to_string(turn++) + "]", managerCandidatesPendingToDoctor, managerCandidatesPendingToDoctor + _managersCount - 1);
	}
}

void BeautyAgent::waitInDoctor()
{
	if (_id == 0)
	{
		{
			std::unique_lock<std::mutex> ll(_passingLock);
			*_doctorComplete = PassingState::ACCEPT;
		}

		printf("[Agent %i] start of doctor visit\n", _id);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		printf("[Agent %i] end of doctor visit\n", _id);

		{
			std::unique_lock<std::mutex> ll(_passingLock);
			*_doctorComplete = PassingState::REJECT;
		}
	}
}

void BeautyAgent::checkInSaloon()
{
	std::vector<int> earlierInQueue;
	std::vector<int> afterInQueue;

	int maxCapacity = _saloonCapacity;

	printVector("[Agent " + std::to_string(_id) + "] local queue", _queueToSaloon, _queueToSaloon + _managersCount - 1);

	for (int i = 0; i < _managersCount; ++i)
		if (_queueToSaloon[i] < _queueToSaloon[_id])
		{
			printf("[Agent %i] agent %i is before me\n", _id, i);
			earlierInQueue.push_back(i);
		}
		else if (i != _id && _managersCandidatesCount[i] > 0)
			afterInQueue.push_back(i);

	printf("[Agent %i] waiting for saloon\n", _id);

	while(true)
	{
		while (_broker->isAvailable<ReserveSaloon>())
		{
			auto reservation = std::dynamic_pointer_cast<ReserveSaloon>(_broker->receive<ReserveSaloon>());

			auto sender = std::find(earlierInQueue.begin(), earlierInQueue.end(), reservation->getManagerId());
			if (sender != earlierInQueue.end())
			{
				_saloonCapacity -= _managersCandidatesCount[reservation->getManagerId()];
				earlierInQueue.erase(sender);
			}

			printf("[Agent %i] Another agent %i reserved [%i/%i]\n", _id, reservation->getManagerId(), _saloonCapacity, maxCapacity);
		}

		while (_broker->isAvailable<FreeSaloon>())
		{
			auto reservation = std::dynamic_pointer_cast<FreeSaloon>(_broker->receive<FreeSaloon>());
			_saloonCapacity += _managersCandidatesCount[reservation->getManagerId()];
			printf("[Agent %i] Agent %i free [%i/%i]\n", _id, reservation->getManagerId(), _saloonCapacity, maxCapacity);
		}

		for (const int earlier : earlierInQueue)
		{
			printf("[Agent %i] Asking if %i let us pass\n", _id, earlier);
			auto passMeRequest = PassMeRequest(_id);
			_broker->send(passMeRequest, { earlier });
			auto request = std::dynamic_pointer_cast<PassMeDecision>(_broker->receive<PassMeDecision>());

			if (request->getDecision() == (int)PassingState::REJECT)
			{
				printf("[Agent %i] Agent %i refused passing [%i]\n", _id, earlier, request->getDecision());
				_saloonCapacity -= _managersCandidatesCount[earlier];
			}
			else
			{
				printf("[Agent %i] Agent %i lets us go before him\n", _id, earlier);
				afterInQueue.push_back(earlier);
			}
		}

		earlierInQueue.clear();

		if (_saloonCapacity >= _candidatesCount)
		{
			printf("[Agent %i] Reservation [%i\\%i]\n", _id, _saloonCapacity, maxCapacity);
			auto reservation = ReserveSaloon(_id);

			_saloonCapacity -= _candidatesCount;
			_broker->send(reservation, afterInQueue);

			break;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	printf("[Agent %i] start of saloon visit [%i\\%i]\n", _id, _saloonCapacity, maxCapacity);
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 1000 + 300));
	_saloonCapacity += _candidatesCount;
	printf("[Agent %i] end of saloon visit [%i\\%i]\n", _id, _saloonCapacity, maxCapacity);

	auto freeSaloon = FreeSaloon(_id);
	_broker->send(freeSaloon, afterInQueue);
}

void BeautyAgent::passerLoop()
{
	while (_running)
	{
		if (_broker->isAvailable<PassMeRequest>())
		{
			std::unique_lock<std::mutex> ll(_passingLock);

			auto request = std::dynamic_pointer_cast<PassMeRequest>(_broker->receive<PassMeRequest>());

			printf("[Agent %i] pass decision for %i %s\n",
				   _id, request->getManagerId(), (*_doctorComplete == PassingState::ACCEPT ? "Accept\0" : "Reject\0"));

			auto decisionMessage = PassMeDecision(*_doctorComplete);

			_broker->send(decisionMessage, {request->getManagerId()});

			if (*_doctorComplete  == PassingState::ACCEPT)
				_queueToSaloon[request->getManagerId()] = -1;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}


