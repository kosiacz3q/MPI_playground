#include <iostream>

#include <iostream>
#include <mpi.h>

#include "Messages.hpp"
#include "BeautyAgent.hpp"

using namespace std;

static int agentsCount;

int main(int argc, char **argv)
{
	int agentId;
	// MPI initializations
	MPI_Status status;
	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &agentsCount);
	MPI_Comm_rank (MPI_COMM_WORLD, &agentId);
	double time_start = MPI_Wtime();

	BeautyAgent ba(agentId, agentsCount);

	ba.run();



	// End MPI
	MPI_Finalize ();
	return 0;
}