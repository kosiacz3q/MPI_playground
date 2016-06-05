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
	MessageBroker _broker;
	int _managersCount;
	int _doctorsCount;
	int _minInDoctorQueue;
	int _saloonCapacity;

	int* _managersCandidatesCount;
	int* _queueToSaloon;
	std::thread _puller;

	void prepare();

	void checkInDoctor();

	void waitForAllManagersToBeReady();
};


#endif //CONTEST_BEAUTYAGENT_HPP
