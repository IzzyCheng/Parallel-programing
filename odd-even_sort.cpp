#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	int myid, numprocs;
	int n=0;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	srand(time(NULL)+myid*numprocs*100);

	//Proc 0 Get n and broadcast
	if (myid == 0)
		scanf("%d", &n);
	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	//Randomly Generate local list
	int local[n/numprocs];
	for (int i = 0; i<n/numprocs; i++)
		local[i] = (rand() % (2*n))+1;	//random from 1 to 2n

	//Gather random local list
	int *globalptr = NULL;
	int global[n];
	if (myid == 0)
		globalptr = &global[0];

	//Sort local list
	for (int length = n/numprocs; length >= 2; length--)
		for (int i = 0; i<length-1; i++)
			if (local[i]>local[i+1])
			{
				int temp = local[i];
				local[i] = local[i+1];
				local[i+1] = temp;
			}
	
	//Gather sorted local list tp proc 0
	MPI_Gather(&local[0], n/numprocs, MPI_INT, globalptr, n/numprocs, MPI_INT, 0, MPI_COMM_WORLD);
	if (myid == 0 )
		for (int proc = 0; proc<numprocs; proc++) {
			printf("proc%d | ", proc);
			for (int i = 0; i<n/numprocs; i++)
				printf("%d ", global[proc*n/numprocs+i]);
			printf("\n");
		}
	MPI_Barrier(MPI_COMM_WORLD);

	//odd-even sort
	for (int phase = 0; phase < numprocs; phase++)
	{
		int partner;
		int recv[n/numprocs];

		//Compute partner
		if (phase % 2 == 0)
		{
			if (myid %2 != 0)
				partner = myid-1;
			else 
				partner = myid+1;
		}
		else
		{
			if (myid %2 != 0)
				partner = myid+1;
			else
				partner = myid-1;
		}
		if (partner == -1 || partner == numprocs)
			partner = MPI_PROC_NULL;
		MPI_Barrier(MPI_COMM_WORLD);

		//Sort with partner
		if (partner != MPI_PROC_NULL)
		{
			//Send and Recv Keys
			MPI_Send(&(local[0]), n/numprocs, MPI_INT, partner, 0, MPI_COMM_WORLD);
			MPI_Recv(&(recv[0]), n/numprocs, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			//Get keys
			if (myid > partner)
			{
				//take larger keys
				for (int i = n/numprocs-1; i>=0; i--)
					for (int j = n/numprocs-1; j>=0; j--)
						if (local[i]<recv[j]) 
						{	//swap local[i], recv[j]
							int temp = local[i];
							local[i] = recv[j];
							recv[j] = temp;
						}
			}
			else
			{
				//take smaller keys
				for (int i = 0; i<n/numprocs; i++)
					for (int j = 0; j<n/numprocs; j++)
						if (local[i]>recv[j])
						{	//swap local[i], recv[j]
							int temp = local[i];
							local[i] = recv[j];
							recv[j] = temp;
						}
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);

		//Gather local list to global list
		MPI_Gather(&local[0], n/numprocs, MPI_INT, globalptr, n/numprocs, MPI_INT, 0, MPI_COMM_WORLD);
		if (myid == 0)
		{
			printf("After Phase %d : ", phase);
			for (int i = 0; i<n; i++)
				printf("%d ", global[i]);
			printf("\n");
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
	
	//Print the result
	MPI_Gather(&local[0], n/numprocs, MPI_INT, globalptr, n/numprocs, MPI_INT, 0, MPI_COMM_WORLD);
	if (myid == 0)
	{
		printf("Result : ");
		for (int i = 0; i<n; i++)
			printf("%d ", global[i]);
		printf("\n");
	}
	
	MPI_Finalize();


	return 0;
}
