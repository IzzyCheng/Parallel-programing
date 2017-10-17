/* circuitSatifiability.c solves the Circuit Satisfiability
 *  *  Problem using a brute-force sequential solution.
 *   *
 *    *   The particular circuit being tested is "wired" into the
 *     *   logic of function 'checkCircuit'. All combinations of
 *      *   inputs that satisfy the circuit are printed.
 *       *
 *        *   16-bit version by Michael J. Quinn, Sept 2002.
 *         *   Extended to 32 bits by Joel C. Adams, Sept 2013.
 *          */

#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include <mpi.h>

int checkCircuit (int, long);

int main (int argc, char *argv[]) 
{
	long i, j = 2;
	int count = 0;
	int numprocs, myid;
	int mycount = 0;
	double startTime = 0.0, totalTime = 0.0;

	MPI_Init(&argc, &argv);					//Initialize MPI
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);		
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	if (myid == 0)						//start to count time
		startTime = MPI_Wtime();

	for (i = myid; i <= UINT_MAX; i += numprocs)		//count part in process
		mycount += checkCircuit (myid, i);

	for (j = 2; j <= numprocs; j*=2)			//tree-structured communication 
	{
		if (myid%j == j/2)				//1 3 5 7 send; 2 6 send; 4 send
			MPI_Send(&mycount, 1, MPI_INT, myid-(j/2), 0, MPI_COMM_WORLD);
		else if(myid%j == 0)				//0 2 4 6 recv; 0 4 recv; 0 recv
		{
			MPI_Recv(&count, 1, MPI_INT, myid+(j/2), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			mycount += count;
		}
	}

	if (myid == 0) {					//print the result
		totalTime = MPI_Wtime() - startTime;
		printf("\nA total of %d solutions were found.\n\n", mycount);
		printf("Finished time = %f secs.\n", totalTime);
		fflush(stdout);
	}
	MPI_Finalize();
	return 0;
}

/* EXTRACT_BIT is a macro that extracts the ith bit of number n.
 *  *
 *   * parameters: n, a number;
 *    *             i, the position of the bit we want to know.
 *     *
 *      * return: 1 if 'i'th bit of 'n' is 1; 0 otherwise 
 *       */

#define EXTRACT_BIT(n,i) ( (n & (1<<i) ) ? 1 : 0)


/* checkCircuit() checks the circuit for a given input.
 *  * parameters: id, the id of the process checking;
 *   *             bits, the (long) rep. of the input being checked.
 *    *
 *     * output: the binary rep. of bits if the circuit outputs 1
 *      * return: 1 if the circuit outputs 1; 0 otherwise.
 *       */

#define SIZE 32

int checkCircuit (int id, long bits) {
	int v[SIZE];        /* Each element is a bit of bits */
	int i;

	for (i = 0; i < SIZE; i++)
		v[i] = EXTRACT_BIT(bits,i);

	if ( ( (v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
				&& (!v[3] || !v[4]) && (v[4] || !v[5])
				&& (v[5] || !v[6]) && (v[5] || v[6])
				&& (v[6] || !v[15]) && (v[7] || !v[8])
				&& (!v[7] || !v[13]) && (v[8] || v[9])
				&& (v[8] || !v[9]) && (!v[9] || !v[10])
				&& (v[9] || v[11]) && (v[10] || v[11])
				&& (v[12] || v[13]) && (v[13] || !v[14])
				&& (v[14] || v[15]) )
			||
			( (v[16] || v[17]) && (!v[17] || !v[19]) && (v[18] || v[19])
			  && (!v[19] || !v[20]) && (v[20] || !v[21])
			  && (v[21] || !v[22]) && (v[21] || v[22])
			  && (v[22] || !v[31]) && (v[23] || !v[24])
			  && (!v[23] || !v[29]) && (v[24] || v[25])
			  && (v[24] || !v[25]) && (!v[25] || !v[26])
			  && (v[25] || v[27]) && (v[26] || v[27])
			  && (v[28] || v[29]) && (v[29] || !v[30])
			  && (v[30] || v[31]) ) )
	{
		printf ("%d) %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d \n", id,
				v[31],v[30],v[29],v[28],v[27],v[26],v[25],v[24],v[23],v[22],
				v[21],v[20],v[19],v[18],v[17],v[16],v[15],v[14],v[13],v[12],
				v[11],v[10],v[9],v[8],v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);
		fflush (stdout);
		return 1;
	} else {
		return 0;
	}
}


