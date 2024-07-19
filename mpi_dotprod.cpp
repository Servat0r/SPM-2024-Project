#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include "mpi.h"
#include "utils.hpp"


/**
 * Dot product between two arrays
 */
double diffmsec(const struct timeval & a, 
				const struct timeval & b) {
    long sec  = (a.tv_sec  - b.tv_sec);
    long usec = (a.tv_usec - b.tv_usec);
    
    if(usec < 0) {
        --sec;
        usec += 1000000;
    }
    return ((double)(sec*1000)+ (double)usec/1000.0);
}

// Function to compute product for a portion of the array
int main(int argc, char* argv[]){
	int myid, numprocs, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	double t0, t1;
	struct timeval wt1, wt0;
    uint64_t N = std::stol(argv[1]);
	
	// MPI_Wtime cannot be used here
	gettimeofday(&wt0, NULL);
	MPI_Init(&argc, &argv);	
	t0 = MPI_Wtime();
	
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs); 
	MPI_Comm_rank(MPI_COMM_WORLD, &myid); 
	MPI_Get_processor_name(processor_name, &namelen);

    if (myid == 0){
        std::cout << "Server with id = " << myid << " on " << processor_name << std::endl;
        std::vector<long> x(N, 0);
        std::vector<long> y(N, 0);
        std::vector<long> z(N, 0);
        for (uint64_t i = 0; i < N; i++){
            x[i] = (long)i;
            y[i] = (long)i;
        }
        uint64_t blockSize = N / ((uint64_t)numprocs - 1);
        std::cout << "BlockSize = " << blockSize << std::endl;
        for (int i = 1; i < numprocs; i++){
            int start = (i-1) * (int)blockSize;
            int end = std::min(start + (int)blockSize, (int)N);
            if (i + 1 == numprocs) end = (int)N;
            std::cout << "For worker " << i << " we have start = " << start << " end = " << end << std::endl;
            MPI_Send(
                x.data() + start, end - start, MPI_LONG,
                i, 3*i, MPI_COMM_WORLD
            );
            MPI_Send(
                y.data() + start, end - start, MPI_LONG,
                i, 3*i+1, MPI_COMM_WORLD
            );
            MPI_Recv(
                z.data() + start, end - start, MPI_LONG,
                i, 3*i+2, MPI_COMM_WORLD, MPI_STATUS_IGNORE
            );
            printVector(z, NULL);
        }
    } else {
        std::cout << "Worker with id = " << myid << " on " << processor_name << std::endl;
        uint64_t blockSize = N / ((uint64_t)numprocs - 1);
        int start = (myid-1) * (int)blockSize;
        int end = std::min(start + (int)blockSize, (int)N);
        if (myid + 1 == numprocs) end = (int)N;
        std::vector<long> x(blockSize, 0);
        std::vector<long> y(blockSize, 0);
        std::vector<long> z(blockSize, 0);
        std::cout << "Worker with id = " << myid << " will receive " << end - start << " items" << std::endl;
        MPI_Recv(
            x.data(), end - start, MPI_LONG, 0, 3*myid, MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );
        MPI_Recv(
            y.data(), end - start, MPI_LONG, 0, 3*myid + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );
        printVector(x, NULL);
        printVector(y, NULL);
        for (uint64_t i = 0; i < (uint64_t)(end - start); i++) z[i] = x[i] * y[i];
        MPI_Send(
            z.data(), end - start, MPI_LONG, 0, 3*myid + 2, MPI_COMM_WORLD
        );
        std::cout << "Worker with id = " << myid << " has terminated" << std::endl;
    }

	t1 = MPI_Wtime();
	
	MPI_Finalize();
	gettimeofday(&wt1,NULL);  

	std::cout << "Total time (MPI) " << myid << " is " << t1-t0 << " (S)\n";
	std::cout << "Total time       " << myid << " is " << diffmsec(wt1,wt0)/1000 << " (S)\n";
	return 0;
}