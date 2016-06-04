//
// Created by lucas on 04.06.16.
//

#ifndef CONTEST_BEAUTYAGENT_HPP
#define CONTEST_BEAUTYAGENT_HPP


#include "MessageBroker.hpp"

class BeautyAgent
{
public:

	BeautyAgent(const int id, const int managersCount);

	virtual ~BeautyAgent();

	void run();

	static int MaxCandidatesCount;

private:

	int _id;
	int* _managersCandidatesCount;
	MessageBroker _broker;

	void prepare();

};


#endif //CONTEST_BEAUTYAGENT_HPP
