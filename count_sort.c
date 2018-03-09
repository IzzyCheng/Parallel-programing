#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]){
	int i, j, count;
	int thread_count = strtol(argv[1], NULL, 10);
	int n = strtol(argv[2], NULL, 10);
	int *a = malloc(n*sizeof(int));
	int *temp = malloc(n*sizeof(int));
	double start, end;
	srand(time(NULL));

	//random from 1 to n
	for (i=0; i<n; i++)
		a[i] = (rand()%n) + 1;

	start = omp_get_wtime();
	//count_sort
#pragma omp parallel for num_threads(thread_count) default(none) private(i, j, count) shared(a, n, temp)
	for (i=0; i<n; i++) {
		count = 0;
		for (j=0; j<n; j++)
			if (a[j] < a[i])
				count++;
			else if (a[j] == a[i] && j<i)
				count++;
		temp[count] = a[i];
	}

	memcpy(a, temp, n*sizeof(int));
	free(temp);
	end = omp_get_wtime();

	printf("Time : %f sec\n", end-start);
	return 0;
}
