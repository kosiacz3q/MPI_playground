#include <iostream>

#include <iostream>
#include <mpi.h>
#include <algorithm>
#include <locale>

#include "Messages.hpp"
#include "BeautyAgent.hpp"
#include "Logger.hpp"

bool checkArgsCorrectness(int argc, char **argv);

int main(int argc, char **argv)
{
	int provided;
	int agentId;
	int agentsCount;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
	MPI_Comm_size (MPI_COMM_WORLD, &agentsCount);
	MPI_Comm_rank (MPI_COMM_WORLD, &agentId);

	if (provided < MPI_THREAD_SERIALIZED)
	{
		if (agentId == 0)
			printf("Minimum requirements for multithreading are not met\n");

		MPI_Finalize ();
		return 1;
	}

	if (!checkArgsCorrectness(argc, argv))
	{
		if (agentId == 0)
			printf("Missing or incorrect parameters\n"
			"run with <max_candidates:u_int> <doctors_count:u_int> <saloon_capacity:u_int> <log lammport clock:[0/1]>\n");

		MPI_Finalize ();
		return 2;
	}

	Message::SetLamportClockSize(agentsCount);
	MessageBroker::lamportClock = std::make_shared<LamportVectorClock>(agentsCount, agentId);
	Logger::lamportLoggingEnabled = (argv[4][0] == '1');
	Logger::lamportVectorClock = MessageBroker::lamportClock;
	BeautyAgent::MaxCandidatesCount = std::stoi(argv[1]);


	unsigned int contestNumber = 0;
	while((contestNumber = ++contestNumber % std::numeric_limits<unsigned int>::max()))
	{
		BeautyAgent ba(agentId, agentsCount, std::stoi(argv[2]), std::stoi(argv[3]), contestNumber);

		ba.run();
	}

	MPI_Finalize ();

	printf("[Program %i] ends\n", agentId);

	return 0;
}

bool checkArgsCorrectness(int argc, char **argv)
{
	if (argc != 5)
		return false;

	for (int i = 1; i != 4; ++i)
	{
		auto param = std::string(argv[i]);
		std::string::const_iterator it = param.begin();
		while (it != param.end())
		{
			if (!std::isdigit(*it))
				return false;

			++it;
		}
	}

	return (argv[4][0] == '0' || argv[4][0] == '1');
}