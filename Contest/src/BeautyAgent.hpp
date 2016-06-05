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
	int* _managersCandidatesCount;
	int* _queueToSaloon;

	MessageBroker _broker;

	void prepare();

	void checkInDoctor();

	void waitForAllManagersToBeReady();

	std::thread _puller;

	int _managersCount;
	int _doctorsCount;
	int _minInDoctorQueue;
	int _saloonCapacity;
};


#endif //CONTEST_BEAUTYAGENT_HPP
