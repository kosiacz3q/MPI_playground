#include <iostream>

#include <iostream>
#include <mpi.h>

#include "Messages.hpp"
#include "BeautyAgent.hpp"

int main(int argc, char **argv)
{
	//MPI_Init (&argc, &argv);

	int provided;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

	//printf("Support for multiple threads [%s]\n", provided == MPI_THREAD_MULTIPLE ? "yes" : "no");

	int agentId;
	int agentsCount;

	MPI_Comm_size (MPI_COMM_WORLD, &agentsCount);
	MPI_Comm_rank (MPI_COMM_WORLD, &agentId);

	Message::SetLamportClockSize(agentsCount);

	BeautyAgent ba(agentId, agentsCount, 1, BeautyAgent::MaxCandidatesCount);

	ba.run();

	MPI_Finalize ();

	printf("[Prog %i] end\n", agentId);

	return 0;
}