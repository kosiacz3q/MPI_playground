//
// Created by lucas on 14.06.16.
//

#include "LamportVectorClock.hpp"
#include <iostream>
#include <sstream>

LamportVectorClock::LamportVectorClock(const int size, const int id)
	:_clockVector(std::vector<int>(size)), _id(id)
{

}

void LamportVectorClock::update(const std::vector<int> otherVector)
{
	std::unique_lock<std::mutex> ll(_clockMutex);

	for (int i = 0; i < _clockVector.size(); ++i)
		if (_clockVector[i] < otherVector[i])
			_clockVector[i] = otherVector[i];
}

std::vector<int> LamportVectorClock::getCurrent()
{
	std::unique_lock<std::mutex> ll(_clockMutex);
	return _clockVector;
}

void LamportVectorClock::update()
{
	std::unique_lock<std::mutex> ll(_clockMutex);
	++_clockVector[_id];
}

std::ostringstream &operator<<(std::ostringstream &os, LamportVectorClock &obj)
{
	auto _clockVector = obj.getCurrent();

	os<< " [";

	for (int lc : _clockVector)
		os<< lc << ",";

	os<< "]";

	return os;
}










