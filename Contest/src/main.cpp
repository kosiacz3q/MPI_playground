#include <iostream>

#include <iostream>
#include <mpi.h>

#include "Messages.hpp"
#include "BeautyAgent.hpp"

using namespace std;

static int agentsCount;

int main(int argc, char **argv)
{
	MPI_Init (&argc, &argv);

	int agentId;

	MPI_Comm_size (MPI_COMM_WORLD, &agentsCount);
	MPI_Comm_rank (MPI_COMM_WORLD, &agentId);

	BeautyAgent ba(agentId, agentsCount);

	ba.run();

	// End MPI
	MPI_Finalize ();

	printf("End\n");

	return 0;
}