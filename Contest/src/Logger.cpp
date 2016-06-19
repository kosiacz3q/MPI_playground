#include <cstdio>
#include <string>
#include "Logger.hpp"


LamportVectorClockPtr Logger::lamportVectorClock;
bool Logger::lamportLoggingEnabled = false;

void Logger::log(const std::string sender, const std::string message)
{
	printf("%-3s [%s] %s\n", lamportVectorClock->getForCurrentAgent().c_str(), sender.c_str(), message.c_str());
}
