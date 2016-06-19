#include "BeautyAgent.hpp"
#include "Logger.hpp"

#include <utils.h>
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

	_doctorComplete = std::make_shared<PassingState>(PassingState::ACCEPT);

	_puller = std::thread(&MessageBroker::pullMessages, &*_broker);
	_passer = std::thread(&BeautyAgent::passerLoop, this);

	log("ready for action");
}

void BeautyAgent::run()
{
	prepare();

	if (_candidatesCount != 0)
	{
		checkInDoctor();

		checkInSaloon();
	}
	else
	{
		log("Is not participating in current contest edition");
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
}

void BeautyAgent::prepare()
{
	std::srand((unsigned int) (std::time(0) + _id));

	_managersCandidatesCount[_id] = _candidatesCount = 1;//std::rand() % MaxCandidatesCount + 1;

	log("Choosen counf of candidates is " + std::to_string( _managersCandidatesCount[_id]));

	auto participationMessage = ParticipationMessage(_id, _managersCandidatesCount[_id]);
	_broker->send(participationMessage);

	int i = _managersCount;

	while (--i)
	{
		auto agentData = std::dynamic_pointer_cast<ParticipationMessage>(_broker->receive<ParticipationMessage>());
		_managersCandidatesCount[agentData->getManagerId()] = agentData->getCandidatesCount();
	}

	if (_id == 0)
		printf("====================================================================\n"
			"=======================NEW CONTEST STARTED==========================\n"
			"====================================================================\n");

	log("Candidates count for all managers" + printVector(_managersCandidatesCount, _managersCandidatesCount + _managersCount - 1));
}

void BeautyAgent::waitForAllManagersToBeReady()
{
	int i = _managersCount - 1;

	auto readyMessage = AgentReadyToContestMessage();
	_broker->send(readyMessage);

	log("Waiting for others");

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

	log("End of waiting for others");
}

void BeautyAgent::checkInDoctor()
{
	int doctorsQueue[_doctorsCount];
	auto targetsToSend = std::vector<int>();
	auto queueParticipants = std::vector<int>();
	auto managersInDoctor = std::vector<int>();

	int managerCandidatesPendingToDoctor[_managersCount];
	memcpy(managerCandidatesPendingToDoctor, _managersCandidatesCount, _managersCount * sizeof(int));

	while(managerCandidatesPendingToDoctor[_id])
	{
		if (!managersInDoctor.empty())
			log("Waiting for visits to end");

		while (!managersInDoctor.empty())
		{
			auto response = std::dynamic_pointer_cast<DoctorsVisitEnds>(_broker->receive<DoctorsVisitEnds>());
			auto awaitedResponse = std::find(managersInDoctor.begin(), managersInDoctor.end(), response->getManagerId());

			if (awaitedResponse ==  managersInDoctor.end())
			{
				_broker->retract(response);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			managersInDoctor.erase(awaitedResponse);
		}

		log("Trying to reach doctor");
		//clear queue
		for (int i = 0; i < _doctorsCount; ++i)
			doctorsQueue[i] = std::numeric_limits<int>::max();

		// query other managers who have non checked candidates
		targetsToSend.clear();
		for (int i = 0; i < _managersCount; ++i)
			if (managerCandidatesPendingToDoctor[i] > 0 && i != _id)
				targetsToSend.push_back(i);

		queueParticipants = targetsToSend;

		// send message with choosen doctor
		int choosenDoctor = std::rand() % _doctorsCount;
		auto sendToDoctorMessage = SendToDoctorMessage(_id, choosenDoctor);
		doctorsQueue[choosenDoctor] = _id;

		_broker->send(sendToDoctorMessage, targetsToSend);

		log("Choosen doctor is "+ std::to_string(choosenDoctor));

		// receive chooses of others
		int expectedResponsesCount = (int) targetsToSend.size();

		while(expectedResponsesCount-- > 0)
		{
			while (!targetsToSend.empty())
			{
				auto response = std::dynamic_pointer_cast<SendToDoctorMessage>(_broker->receive<SendToDoctorMessage>());

				auto awaitedResponse = std::find(targetsToSend.begin(), targetsToSend.end(), response->getManagerId());

				if (awaitedResponse ==  targetsToSend.end())
				{
					_broker->retract(response);
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}

				if (doctorsQueue[response->getDoctorId()] > response->getManagerId())
					doctorsQueue[response->getDoctorId()] = response->getManagerId();

				targetsToSend.erase(awaitedResponse);
			}
		}

		log("Responses from ohers fetched");

		// perform doctor check
		for (const int managerInDoctor : doctorsQueue)
			if (managerInDoctor != std::numeric_limits<int>::max())
			{
				if (--managerCandidatesPendingToDoctor[managerInDoctor] == 0 && _queueToSaloon[managerInDoctor] != -1)
				{
					_queueToSaloon[managerInDoctor] = _minInDoctorQueue++;

					auto willNotParticipateNextTime = std::find(queueParticipants.begin(), queueParticipants.end(), managerInDoctor);
					if (willNotParticipateNextTime != queueParticipants.end())
						queueParticipants.erase(willNotParticipateNextTime);
				}

				if (managerInDoctor != _id)
					managersInDoctor.push_back(managerInDoctor);
			}

		if (doctorsQueue[choosenDoctor] == _id)
		{
			log("I manage to get to the doctor with number " + std::to_string(choosenDoctor));
			waitInDoctor();
			auto doctorsVisitEnds = DoctorsVisitEnds(_id, choosenDoctor);
			log("Sending end visit to " + std::to_string(queueParticipants.size()) + " queue participants");
			_broker->send(doctorsVisitEnds, queueParticipants);
		}
		else
		{
			log("I did not manage to get to the doctor with number " + std::to_string(choosenDoctor));
		}
	}
}

void BeautyAgent::waitInDoctor()
{
	int visitDuration = (std::rand() % 1000) + (_id == 0 ? 3000 : 0);
	log("Visit in doctor started, estimated duration time is " + std::to_string(visitDuration));
	std::this_thread::sleep_for(std::chrono::milliseconds(visitDuration));
	log("Visit in doctor ended");
}

void BeautyAgent::checkInSaloon()
{
	{
		std::unique_lock<std::mutex> ll(_passingLock);
		*_doctorComplete = PassingState::REJECT;
	}
	log("No longer letting others ahead");

	log("Number in saloon queue " + std::to_string(_queueToSaloon[_id]));

	std::vector<int> earlierInQueue;
	std::vector<int> afterInQueue;

	_maxCapacity = _saloonCapacity;

	log("Local queue" + printVector(_queueToSaloon, _queueToSaloon + _managersCount - 1));

	for (int i = 0; i < _managersCount; ++i)
		if (_queueToSaloon[i] < _queueToSaloon[_id])
		{
			log("Agent " + std::to_string(i) +" is before me");
			earlierInQueue.push_back(i);
		}
		else if (i != _id && _managersCandidatesCount[i] > 0)
			afterInQueue.push_back(i);

	log("Waiting for saloon");

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

			log("Agent " + std::to_string(reservation->getManagerId()) + " reserved "
				"[" + std::to_string(_saloonCapacity) + "/ " + std::to_string(_maxCapacity) + "]");
		}

		while (_broker->isAvailable<FreeSaloon>())
		{
			auto reservation = std::dynamic_pointer_cast<FreeSaloon>(_broker->receive<FreeSaloon>());
			_saloonCapacity += _managersCandidatesCount[reservation->getManagerId()];

			log("Agent " + std::to_string(reservation->getManagerId()) + " free "
					"[" + std::to_string(_saloonCapacity) + "/ " + std::to_string(_maxCapacity) + "]");
		}

		for (const int earlier : earlierInQueue)
		{
			log("Asking if " + std::to_string(earlier) + " let us pass");
			auto passMeRequest = PassMeRequest(_id);
			_broker->send(passMeRequest, { earlier });
			auto request = std::dynamic_pointer_cast<PassMeDecision>(_broker->receive<PassMeDecision>());

			if (request->getDecision() == (int)PassingState::REJECT)
			{
				log("Agent " + std::to_string(earlier)  + " refused passing");
				_saloonCapacity -= _managersCandidatesCount[earlier];
			}
			else
			{
				log("Agent " + std::to_string(earlier)  + " lets us go before him");
				afterInQueue.push_back(earlier);
			}
		}

		earlierInQueue.clear();

		if (_saloonCapacity >= _candidatesCount)
		{
			auto reservation = ReserveSaloon(_id);
			_saloonCapacity -= _candidatesCount;
			log("Reserving place in saloon [" + std::to_string(_saloonCapacity)  + "/" + std::to_string(_maxCapacity) + "]");
			_broker->send(reservation, afterInQueue);

			waitInSaloon();

			_saloonCapacity += _candidatesCount;
			log("Freeing place in saloon [" + std::to_string(_saloonCapacity)  + "/" + std::to_string(_maxCapacity) + "]");

			auto freeSaloon = FreeSaloon(_id);
			_broker->send(freeSaloon, afterInQueue);

			return;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

void BeautyAgent::passerLoop()
{
	while (_running)
	{
		if (_broker->isAvailable<PassMeRequest>())
		{
			auto request = std::dynamic_pointer_cast<PassMeRequest>(_broker->receive<PassMeRequest>());

			{
				std::unique_lock<std::mutex> ll(_passingLock);

				if (*_doctorComplete == PassingState::ACCEPT)
				{
					_queueToSaloon[request->getManagerId()] = -1;
				}
			}

			log("Pass decision for " + std::to_string(request->getManagerId()) + " is " +
				(*_doctorComplete == PassingState::ACCEPT ? "Accept\0" : "Reject\0"));

			auto decisionMessage = PassMeDecision(*_doctorComplete);

			_broker->send(decisionMessage, {request->getManagerId()});
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

void BeautyAgent::log(const std::string &message)
{
	static std::string whoami = "Agent " + std::to_string(_id);

	Logger::log(whoami, message);
}

void BeautyAgent::waitInSaloon()
{
	int visitDuration = (std::rand() % 2000);

	log("Start of saloon visit [" + std::to_string(_saloonCapacity)  + "/" + std::to_string(_maxCapacity) + "] estimated duration time is " + std::to_string(visitDuration));
	std::this_thread::sleep_for(std::chrono::milliseconds(visitDuration));
	log("End of saloon visit [" + std::to_string(_saloonCapacity)  + "/" + std::to_string(_maxCapacity) + "]");
}






