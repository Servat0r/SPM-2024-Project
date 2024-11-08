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
	/* Work for a single matrix cell */
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
	/**
	 * Work for a rectangular tile defined by the coordinates [minX, maxX] x [minY, maxY]
	 * X = rows, Y = columns
	*/
	double value;
	for (uint64_t i = maxX; i >= minX; i--){
		for (uint64_t j = minY; j <= maxY; j++){
			uint64_t i_offset = i - minX;
			uint64_t j_offset = j - minY;
			if (i_offset > j_offset){
				if (K + j_offset < i_offset) continue; 
			}
			int k = K - i_offset + j_offset;
			if (k >= 1 && k < N){
				value = work((uint64_t)k, (uint64_t)i, M, N);
				computedData[pos] = value;
				pos++;
			}
		}
		if (i == 0) break;
	}
	return pos;
}

void sequentialTileWork(uint64_t minX, uint64_t minY, uint64_t maxX, uint64_t maxY,
	std::vector<double> &M, const uint64_t &N, uint64_t K){
	/* Tile work adapted for the sequential version */
	double value;
	for (int i = maxX; i >= (int)minX; i--){
		for (uint64_t j = minY; j <= maxY; j++){
			int k = K - (i - minX) + (j - minY);
			if (k >= 1){
				value = work((uint64_t)k, (uint64_t)i, M, N);
			}
		}
	}
}

void unpackData(
	std::vector<double> &M, const uint64_t &N,
	uint64_t start, uint64_t end, uint64_t tileSize,
	std::vector<double> &computedData, uint64_t K
){
	/* Unpacks data from an array to the result matrix */
	uint64_t pos = 0;
	uint64_t minX, minY, maxX, maxY;
	for (uint64_t i = start; i < end; i++) {
		minX = tileSize * i;
		minY = minX + K;
		maxX = std::min(minX + tileSize - 1, N - 1);
		maxY = std::min(minY + tileSize - 1, N - 1);
		for (int l = maxX; l >= (int)minX; l--){
			for (uint64_t j = minY; j <= maxY; j++){
				int k = K - (l - minX) + (j - minY);
				if (k >= 1){
					M[l*N + (l+k)] = computedData[pos];
					pos++;
				}
			}
		}
	}
}

uint64_t getTotalDiagonalSize(uint64_t numTiles, uint64_t tileSize, uint64_t N, uint64_t K){
	/* Returns the total cell size of a tile-made diagonal */
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

// Sequential version
void sequentialWavefront(std::vector<double> &M, const uint64_t &N, const uint64_t tileSize) {
	for (uint64_t K = 0; K < N; K += tileSize){
		uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
		uint64_t pos = 0;
		uint64_t minX, maxX, minY, maxY;
		for (uint64_t i = 0; i < numTiles; i++){
			minX = tileSize * i;
			minY = minX + K;
			maxX = std::min(minX + tileSize - 1, N - 1);
			maxY = std::min(minY + tileSize - 1, N - 1);
			sequentialTileWork(minX, minY, maxX, maxY, M, N, K);
		}
	}
}


int main(int argc, char* argv[]){
	int myid, nworkers, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	double t0, t1;
	struct timeval wt1, wt0;
    uint64_t N = argc > 1 ? std::stol(argv[1]) : 2000;
    uint64_t tileSize = argc > 2 ? std::stol(argv[2]) : 1;
	uint64_t policy = argc > 3 ? std::stol(argv[3]) : 1;
	uint64_t nnodes = argc > 4 ? std::stol(argv[4]) : 0;
	std::string filename = argc > 5 ? argv[5] : "output_results_mpi.csv";
	
	// MPI_Wtime cannot be used here
	gettimeofday(&wt0, NULL);
	MPI_Init(&argc, &argv);	
	t0 = MPI_Wtime();
	
	MPI_Comm_size(MPI_COMM_WORLD, &nworkers);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Get_processor_name(processor_name, &namelen);
	if (nnodes == 0) nnodes = (uint64_t)nworkers;
	
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
		// K is the current 1-sized diagonal at the top left of the current "tile diagonal"
		for (uint64_t K = 0; K < N; K += tileSize){
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize; // Number of tiles in the current tile diagonal
			uint64_t baseBlockSize = numTiles / (nworkers - 1); // Number ot tiles to be given to each worker
			if (baseBlockSize == 0) baseBlockSize = 1;
			uint64_t start = (myid - 1) * baseBlockSize; // start block
			uint64_t end; // end block
			if (myid + 1 < (int)nworkers)
				end = std::min((myid) * baseBlockSize, numTiles);
			else
				end = numTiles;
			if (start < end){
				uint64_t minX, minY, maxX, maxY;
				// Actual tile size is needed for computing size of the vector to be sent to the master
				uint64_t actualTileSize = 0;
				for (uint64_t i = start; i < end; i++){
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
				// Send computed data to master
				MPI_Send(
					computedData.data(), (int)actualTileSize, MPI_DOUBLE, 0,
					K * (2 * nworkers) + myid, MPI_COMM_WORLD
				);
			}
			// Now receive total diagonal data from master
			uint64_t totalDiagonalSize = getTotalDiagonalSize(numTiles, tileSize, N, K);
			std::vector<double> diagonalData(totalDiagonalSize, 0.0);
			if (totalDiagonalSize > 0) MPI_Recv(
				diagonalData.data(), (int)(N * N), MPI_DOUBLE, 0,
				K * (2 * nworkers) + nworkers + myid,
				MPI_COMM_WORLD, MPI_STATUS_IGNORE
			);
			// Update local copy of the matrix
			unpackData(M, N, 0, numTiles, tileSize, diagonalData, K);
		}
	};

	auto serverTask = [&](std::vector<double> &M, const uint64_t &N, uint64_t nworkers, long chunk){
		for (uint64_t K = 0; K < N; K += tileSize){
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			uint64_t baseBlockSize = numTiles / (nworkers - 1);
			if (baseBlockSize == 0) baseBlockSize = 1;
			std::vector<double> diagonalData;
			// First step: receive all data from workers
			for (int myid = 1; myid < (int)nworkers; myid++){
				uint64_t start = (myid - 1) * baseBlockSize; // start block
				uint64_t end; // end block
				if (myid + 1 < (int)nworkers)
					end = std::min((myid) * baseBlockSize, N - K);
				else
					end = N - K;
				if (start < end){
					uint64_t minX, minY, maxX, maxY;
					uint64_t actualTileSize = 0;
					for (uint64_t i = start; i < end; i++){
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
					if (actualTileSize > 0) MPI_Recv(
						computedData.data(), (int)actualTileSize, MPI_DOUBLE, myid,
						K * (2 * nworkers) + myid, MPI_COMM_WORLD, MPI_STATUS_IGNORE
					);
					unpackData(M, N, start, end, tileSize, computedData, K);
					diagonalData.insert(diagonalData.end(), computedData.begin(), computedData.end());
				}
			}
			// Now send all diagonal to all workers
			uint64_t totalDiagonalSize = diagonalData.size();
			for (int myid = 1; myid < (int)nworkers; myid++){
				MPI_Send(
					diagonalData.data(), (int)(totalDiagonalSize), MPI_DOUBLE, myid,
					K * (2 * nworkers) + nworkers + myid, MPI_COMM_WORLD
				);
			}
		}
	};

	std::ofstream output_file;
	if (myid == 0){
		output_file.open(filename, std::ios_base::app);
		if (policy == 1){
			serverTask(M, N, nworkers, tileSize);
		}
	} else {
		if (policy == 1){
			workerTask(M, N, nworkers, tileSize, myid);
		}
	}

	t1 = MPI_Wtime();
	MPI_Finalize();
	gettimeofday(&wt1,NULL);
	
	if (myid == 0){
		std::cout << "Parameters: N = " << N << " policy = " << policy << " nnodes = " << nnodes
		<< " ntasks = " << nworkers - 1 << " tileSize = " << tileSize << std::endl;
		std::cout << "Total time (MPI) " << myid << " is " << 1000.0*(t1-t0) << " (ms)\n";
		std::cout << "Total time       " << myid << " is " << diffmsec(wt1,wt0) << " (ms)\n";
		uint64_t checksum = computeChecksum(M, N);
		std::cout << checksum << std::endl;
		output_file << N << "," << policy << "," << nnodes << "," << nworkers - 1 << "," << tileSize << "," 
			<< 1000.0*(t1-t0) << "," << diffmsec(wt1,wt0) << "," << checksum << std::endl;
	}
	return 0;
}