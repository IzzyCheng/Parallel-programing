#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

int main (int argc, char *argv[]) 
{
	long long int toss, mytoss, myhit=0;
	double x, y, dist, pi, mypi;
	int numprocs, myid, i;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	//input the total toss
	if (myid == 0)
		scanf("%lli", &toss);
	MPI_Bcast(&toss, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);	//brocast toss to other process

	//shoot
	srand(time(NULL));
	for (mytoss = myid; mytoss <= toss; mytoss+=numprocs) 
	{
		x = (2.0)*(double)rand()/RAND_MAX - 1.0;	//random a double from -1 to 1
		y = (2.0)*(double)rand()/RAND_MAX - 1.0;
		dist = x*x + y*y;
		if (dist <= 1.0) myhit++;
	}
	mypi = 4*myhit/((double)toss);

	//sum the mypi and sum together
	for (i = 2; i <= numprocs; i*=2) 
	{
		if (myid%i == i/2)
			MPI_Send(&mypi, 1, MPI_DOUBLE, myid-(i/2), 0, MPI_COMM_WORLD);
		else if (myid%i == 0) 
		{
			MPI_Recv(&pi, 1, MPI_DOUBLE, myid+(i/2), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			mypi += pi;
		}
	}

	//output the result
	if (myid == 0) 
	{
		printf("Total toss :%lli\n", toss);
		printf("The pi_estimate is %.25f\n", mypi);
		fflush(stdout);
	}
	MPI_Finalize();
	return 0;
}
