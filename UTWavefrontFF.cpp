#include <iostream>
#include <barrier>
#include <syncstream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <sstream>
#include <string>
#include <mutex>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include "hpc_helpers.hpp"
#include "utils.hpp"

using namespace ff;
#define MAXWORKERS 16 // Max allowed workers for ParallelFor

// Working function
void work(uint64_t k, uint64_t i, std::vector<double> &M, const uint64_t &N){
	double sum = 0.0;
	uint64_t j1 = i;
	uint64_t i2 = i + k;
	if (i2 < N){
		for (uint64_t h = 0; h < k; h++) sum += M[i*N + (j1 + h)] * M[(i2 - h)*N + (i+k)];
		sum = std::cbrt(sum);
		M[i*N + (i+k)] = sum;
	}
}

// work function computed across a given tile delimited by coordinates: [minX, maxX] x [minY, maxY]
// X = rows, Y = columns
void tileWork(uint64_t minX, uint64_t minY, uint64_t maxX, uint64_t maxY,
	std::vector<double> &M, const uint64_t &N, uint64_t K){
	for (uint64_t i = maxX; i >= minX; i--){
		for (uint64_t j = minY; j <= maxY; j++){
			uint64_t i_offset = i - minX;
			uint64_t j_offset = j - minY;
			if (i_offset > j_offset){
				if (K + j_offset < i_offset) continue; 
			}
			uint64_t k = K - i_offset + j_offset;
			if (k >= 1 && k < N){ work(k, i, M, N); }
		}
		if (i == 0) break;
	}
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
			if (minX < N and minY < N) tileWork(minX, minY, maxX, maxY, M, N, K);
		}
	}
}

// Main body loop
void run(uint64_t N, uint64_t threadNum, uint64_t policy, uint64_t chunkSize,
	uint64_t tileSize, const std::string& filename, uint64_t maxworkers){
	
	// allocate the matrix
	std::vector<double> M(N*N, 0.0);

	std::ofstream output_file;
	output_file.open(filename, std::ios_base::app);

	output_file << N << "," << threadNum << "," << policy << "," << tileSize << "," << chunkSize;

	// init function
	auto init=[&]() {
		for (uint64_t i = 0; i < N; i++){
			M[i*N + i] = (i + 1.) / (double)N;
		}
	};
	
	init();

	auto task = [&](std::vector<double> &M, const uint64_t &N, uint64_t nworkers, long chunk, uint64_t tileSize){
		ParallelFor name(maxworkers);
		for (uint64_t K = 0; K < N; K += tileSize){
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			name.parallel_for(0, numTiles, [&](const uint64_t i){
					// Compute coordinates
					uint64_t minX, minY, maxX, maxY;
					minX = tileSize * i;
					minY = minX + K;
					maxX = std::min(minX + tileSize - 1, N);
					maxY = std::min(minY + tileSize - 1, N);
					if (minX < N and minY < N) tileWork(minX, minY, maxX, maxY, M, N, K);
			}, nworkers);
		}
	};

	// Policy #1: block distribution along a (possibly tiled) diagonal
	auto blockWavefront = [&](std::vector<double> &M, const uint64_t &N,
		uint64_t nworkers, uint64_t tileSize){ task(M, N, nworkers, 0, tileSize); };
	
	// Cyclic distribution policy along a (possibly tiled) diagonal
	auto cyclicWavefront = [&](std::vector<double> &M, const uint64_t &N,
		uint64_t nworkers, uint64_t tileSize){ task(M, N, nworkers, 1, tileSize); };
	
	// Block Cyclic distribution policy along a (possibly tiled) diagonal
	auto blockCyclicWavefront = [&](std::vector<double> &M, const uint64_t &N,
		uint64_t nworkers, uint64_t tileSize, uint64_t chunkSize){ task(M, N, nworkers, chunkSize, tileSize); };
	
	TIMERSTART(wavefront, 1000, "", output_file, ","); // Milliseconds
	std::cout << "Using " << threadNum << " threads" << std::endl;
	// Now spawn the threads and go
	if (policy == 0){
		sequentialWavefront(M, N, tileSize);
	} else if (policy == 1){ // block policy
		blockWavefront(M, N, threadNum, tileSize);
	} else if (policy == 2){ // cyclic policy
		cyclicWavefront(M, N, threadNum, tileSize);
	} else if (policy == 3){
		blockCyclicWavefront(M, N, threadNum, tileSize, chunkSize);
	} else {
		std::cerr << "Error: invalid policy id " << policy << std::endl;
	}
	// join each thread at the end
    TIMERSTOP(wavefront, 1000, "", output_file, ","); // Milliseconds
	output_file << computeChecksum(M, N) << std::endl;
	std::cout << computeChecksum(M, N) << std::endl;
	output_file.close();
}


int main(int argc, char *argv[]) {
	std::string filename = "output_results_ff.csv";
	uint64_t N = argc > 1 ? std::stol(argv[1]) : 1000;
	uint64_t policy = argc > 2 ? std::stol(argv[2]) : 0;
	uint64_t tileSize = argc > 3 ? std::stol(argv[3]) : 1;
	uint64_t threadNum = argc > 4 ? std::stol(argv[4]) : 1;
	uint64_t chunkSize = argc > 5 ? std::stol(argv[5]) : 128;
	uint64_t maxworkers = argc > 6 ? std::stol(argv[6]) : MAXWORKERS;
	if (argc > 7) filename = argv[7];
	std::cout << "N = " << N << " policy = " << policy << " tileSize = " << 
	tileSize << " threadNum = " << threadNum << " chunkSize = " << chunkSize << "\n";

	run(N, threadNum, policy, chunkSize, tileSize, filename, maxworkers);
    return 0;
}