#ifndef CONTEST_LOGGER_HPP
#define CONTEST_LOGGER_HPP


#include "LamportVectorClock.hpp"

class Logger
{
public:
	static LamportVectorClockPtr lamportVectorClock;

	static bool lamportLoggingEnabled;

	static void log(const std::string sender, const std::string message);

};


#endif //CONTEST_LOGGER_HPP
