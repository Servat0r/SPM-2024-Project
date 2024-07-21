#include <iostream>
#include <barrier>
#include <sys/time.h>
#include <unistd.h>
#include <syncstream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <sstream>
#include <string>
#include "hpc_helpers.hpp"
#include "utils.hpp"
#include "mpi.h"

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

double work(uint64_t k, uint64_t i, std::vector<double> &M, const uint64_t &N){
	double sum = 0.0;
	uint64_t j1 = i;
	uint64_t i2 = i + k;
	for (uint64_t h = 0; h < k; h++) sum += M[i*N + (j1 + h)] * M[(i2 - h)*N + (i+k)];
	sum = std::cbrt(sum);
	M[i*N + (i+k)] = sum;
	return sum;
}

uint64_t tileWork(uint64_t minX, uint64_t minY, uint64_t maxX, uint64_t maxY,
	std::vector<double> &M, const uint64_t &N, uint64_t K, std::vector<double> &computedData, uint64_t pos){
	double value;
	for (int i = maxX; i >= (int)minX; i--){
		for (uint64_t j = minY; j <= maxY; j++){
			//std::cout << "i = " << i << " j = " << j << std::endl;
			int k = K - (i - minX) + (j - minY);
			//std::cout << "\tWorking with i = " << i << " j = " << j << " k = " << k << " N = " << N << std::endl;
			if (k >= 1){
				value = work((uint64_t)k, (uint64_t)i, M, N);
				computedData[pos] = value;
				pos++;
			}
		}
	}
	return pos;
}

void unpackData(
	std::vector<double> &M, const uint64_t &N,
	uint64_t start, uint64_t end, uint64_t tileSize,
	std::vector<double> &computedData, uint64_t K
){
	uint64_t pos = 0;
	uint64_t minX, minY, maxX, maxY;
	for (uint64_t i = start; i < end; i++) {
		minX = tileSize * i;
		minY = minX + K;
		maxX = std::min(minX + tileSize - 1, N - 1);
		maxY = std::min(minY + tileSize - 1, N - 1);
		for (int l = maxX; l >= (int)minX; l--){
			for (uint64_t j = minY; j <= maxY; j++){
				//std::cout << "i = " << i << " j = " << j << std::endl;
				int k = K - (l - minX) + (j - minY);
				//std::cout << "\tWorking with i = " << i << " j = " << j << " k = " << k << " N = " << N << std::endl;
				if (k >= 1){
					M[l*N + (l+k)] = computedData[pos];
					pos++;
				}
			}
		}
	}
}

uint64_t getTotalDiagonalSize(uint64_t numTiles, uint64_t tileSize, uint64_t N, uint64_t K){
	uint64_t minX, minY, maxX, maxY;
	uint64_t totalDiagonalSize = 0;
	for (uint64_t i = 0; i < numTiles; i++){
		minX = tileSize * i;
		minY = minX + K;
		maxX = std::min(minX + tileSize, N);
		maxY = std::min(minY + tileSize, N);
		totalDiagonalSize += (maxX - minX) * (maxY - minY);
	}
	return totalDiagonalSize;
}

void sequentialWavefront(std::vector<double> &M, const uint64_t &N) {
	for(uint64_t k = 1; k< N; ++k) {        // for each upper diagonal
		for(uint64_t i = 0; i< (N-k); ++i) {// for each elem. in the diagonal
			work(k, i, M, N);
		}
	}
}


int main(int argc, char* argv[]){
	bool debug = false;
    uint64_t chunkSize = 8;
    uint64_t policy = 1;
	std::string filename = "output_results_mpi.txt";
    
	int myid, nworkers, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	double t0, t1;
	struct timeval wt1, wt0;
    uint64_t N = argc > 1 ? std::stol(argv[1]) : 2000;
    uint64_t tileSize = argc > 2 ? std::stol(argv[2]) : 1;
	
	// MPI_Wtime cannot be used here
	gettimeofday(&wt0, NULL);
	MPI_Init(&argc, &argv);	
	t0 = MPI_Wtime();
	
	MPI_Comm_size(MPI_COMM_WORLD, &nworkers);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Get_processor_name(processor_name, &namelen);
	
	// allocate the matrix
	std::vector<double> M(N*N, 0.0);

	// init function
	auto init=[&]() {
		for (uint64_t i = 0; i < N; i++){
			M[i*N + i] = (i + 1.) / (double)N;
		}
	};
	
	init();

	auto workerTask = [&](std::vector<double> &M, const uint64_t &N, uint64_t nworkers, long tileSize, int myid){
		if (debug) std::cout << "Worker " << myid << " start" << std::endl;
		for (uint64_t K = 0; K < N; K += tileSize){
			if (debug) std::cout << "Worker " << myid << " with K = " << K << std::endl;
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			uint64_t baseBlockSize = numTiles / nworkers;
			if (baseBlockSize == 0) baseBlockSize = 1;
			uint64_t start = (myid - 1) * baseBlockSize; // start block
			uint64_t end; // end block
			if (myid + 1 < (int)nworkers)
				end = std::min(myid * baseBlockSize, numTiles);
			else
				end = numTiles;
			if (start < end){
				uint64_t minX, minY, maxX, maxY;
				uint64_t actualTileSize = 0;
				for (uint64_t i = start; i < end; i++) {
					minX = tileSize * i;
					minY = minX + K;
					maxX = std::min(minX + tileSize, N);
					maxY = std::min(minY + tileSize, N);
					if ((minX <= maxX) && (minY <= maxY)){
						if (K > 0) actualTileSize += (maxX - minX) * (maxY - minY);
						else actualTileSize += (maxX - minX) * (maxY - minY - 1) / 2;
					}
				}
				std::vector<double> computedData(actualTileSize, 0.0);
				uint64_t pos = 0;
				for (uint64_t i = start; i < end; i++){
					minX = tileSize * i;
					minY = minX + K;
					maxX = std::min(minX + tileSize - 1, N - 1);
					maxY = std::min(minY + tileSize - 1, N - 1);
					pos = tileWork(minX, minY, maxX, maxY, M, N, K, computedData, pos);
				}
				if (debug) std::cout << "Worker " << myid << " sending " << actualTileSize << " data to server\n";
				MPI_Send(
					computedData.data(), (int)actualTileSize, MPI_DOUBLE, 0,
					K * (2 * nworkers) + myid, MPI_COMM_WORLD
				);
				if (debug) std::cout << "Worker " << myid << " sent data to server\n";
			}
			uint64_t totalDiagonalSize = getTotalDiagonalSize(numTiles, tileSize, N, K);
			std::vector<double> diagonalData(totalDiagonalSize + 1, 0.0);
			if (debug) std::cout << "Worker " << myid << " receiving " << totalDiagonalSize << " data from server\n";
			if (totalDiagonalSize > 0) MPI_Recv(
				diagonalData.data(), (int)(N*N), MPI_DOUBLE, 0,
				K * (2 * nworkers) + nworkers + myid,
				MPI_COMM_WORLD, MPI_STATUS_IGNORE
			);
			if (debug) std::cout << "Worker " << myid << " received data from server\n";
			unpackData(M, N, 0, numTiles, tileSize, diagonalData, K);
		}
	};

	auto serverTask = [&](std::vector<double> &M, const uint64_t &N, uint64_t nworkers, long chunk){
		if (debug) std::cout << "Server start" << std::endl;
		for (uint64_t K = 0; K < N; K += tileSize){
			if (debug) std::cout << "Server with K = " << K << std::endl;
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			uint64_t baseBlockSize = numTiles / nworkers;
			if (baseBlockSize == 0) baseBlockSize = 1;
			std::vector<double> diagonalData;
			for (int myid = 1; myid < (int)nworkers; myid++){
				uint64_t start = (myid - 1) * baseBlockSize; // start block
				uint64_t end; // end block
				if (myid + 1 < (int)nworkers)
					end = std::min(myid * baseBlockSize, N - K);
				else
					end = N - K;
				if (start < end){
					uint64_t minX, minY, maxX, maxY;
					uint64_t actualTileSize = 0;
					for (uint64_t i = start; i < end; i++) {
						minX = tileSize * i;
						minY = minX + K;
						maxX = std::min(minX + tileSize, N);
						maxY = std::min(minY + tileSize, N);
						if ((minX <= maxX) && (minY <= maxY)){
							if (K > 0) actualTileSize += (maxX - minX) * (maxY - minY);
							else actualTileSize += (maxX - minX) * (maxY - minY - 1) / 2;
						}
					}
					if (debug) std::cout << "Receiving " << actualTileSize << " data from worker " << myid << " with K = " << K << std::endl;
					std::vector<double> computedData(actualTileSize, 0.0);
					if (actualTileSize > 0) MPI_Recv(
						computedData.data(), (int)actualTileSize, MPI_DOUBLE, myid,
						K * (2 * nworkers) + myid, MPI_COMM_WORLD, MPI_STATUS_IGNORE
					);
					if (debug) { std::cout << "Computed Data Received: "; }
					if (debug) std::cout << "Received data from worker " << myid << std::endl;
					unpackData(M, N, start, end, tileSize, computedData, K);
					diagonalData.insert(diagonalData.end(), computedData.begin(), computedData.end());
				}
			}
			uint64_t totalDiagonalSize = diagonalData.size();
			for (int myid = 1; myid < (int)nworkers; myid++){
				if (debug) std::cout << "Sending " << totalDiagonalSize << " data to worker " << myid << "\nDiagonal Data Received: ";
				MPI_Send(
					diagonalData.data(), (int)(totalDiagonalSize), MPI_DOUBLE, myid,
					K * (2 * nworkers) + nworkers + myid, MPI_COMM_WORLD
				);
				if (debug) std::cout << "Sent data to worker " << myid << std::endl;
			}
		}
	};

	std::ofstream output_file;
	if (myid == 0){
		output_file.open(filename, std::ios_base::app);
		output_file << "Parameters: N = " << N << " nworkers = " << nworkers << " policy = " << policy 
			<< " tileSize = " << tileSize << " chunkSize = " << chunkSize << std::endl;
		serverTask(M, N, nworkers, tileSize);
	} else {
		workerTask(M, N, nworkers, tileSize, myid);
	}

	t1 = MPI_Wtime();
	MPI_Finalize();
	gettimeofday(&wt1,NULL);  
	
	if (myid == 0){
		std::cout << "Total time (MPI) " << myid << " is " << t1-t0 << " (S)\n";
		std::cout << "Total time       " << myid << " is " << diffmsec(wt1,wt0)/1000 << " (S)\n";
		std::cout << computeChecksum(M, N) << std::endl;
	}
	return 0;
}