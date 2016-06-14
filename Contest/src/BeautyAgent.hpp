//
// Created by lucas on 04.06.16.
//

#ifndef CONTEST_BEAUTYAGENT_HPP
#define CONTEST_BEAUTYAGENT_HPP

#include <thread>

#include "MessageBroker.hpp"

class BeautyAgent
{
public:

	BeautyAgent(const int id, const int managersCount, const int doctorsCount, const int saloonCapacity);

	virtual ~BeautyAgent();

	void run();

	static int MaxCandidatesCount;

private:

	int _id;
	std::shared_ptr<MessageBroker> _broker;
	int _managersCount;
	int _doctorsCount;
	int _minInDoctorQueue;
	int _saloonCapacity;

	enum PassingState
	{
		ACCEPT = 1,
		REJECT = 2
	};

	std::shared_ptr<PassingState> _doctorComplete;
	bool _running;

	int* _managersCandidatesCount;
	int* _queueToSaloon;

	std::thread _puller;
	std::thread _passer;

	int _candidatesCount;

	std::mutex _passingLock;
	void passerLoop();

	void prepare();

	void checkInDoctor();

	void waitInDoctor();

	void checkInSaloon();

	void waitForAllManagersToBeReady();
};


#endif //CONTEST_BEAUTYAGENT_HPP
