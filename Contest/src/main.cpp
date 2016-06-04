#include <iostream>

#include <iostream>
#include <mpi.h>

#include "Messages.hpp"
#include "BeautyAgent.hpp"

using namespace std;

static int agentsCount;

int main(int argc, char **argv)
{
	//MPI_Init (&argc, &argv);

	int provided;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);


	//printf("Support for multiple threads [%s]\n", provided == MPI_THREAD_MULTIPLE ? "yes" : "no");

	int agentId;

	MPI_Comm_size (MPI_COMM_WORLD, &agentsCount);
	MPI_Comm_rank (MPI_COMM_WORLD, &agentId);

	try
	{
		BeautyAgent ba(agentId, agentsCount);

		ba.run();
	}
	catch (std::bad_alloc& ex)
	{
		printf("[error] %s\n", ex.what());
	}
	// End MPI
	MPI_Finalize ();

	printf("[Prog %i] end\n", agentId);

	return 0;
}