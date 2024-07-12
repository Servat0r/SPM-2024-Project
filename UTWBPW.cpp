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
#include <hpc_helpers.hpp>
#include <utils.hpp>


void work(uint64_t k, uint64_t i, std::vector<double> &M, const uint64_t &N){
	double sum = 0.0;
	uint64_t j1 = i;
	uint64_t i2 = i + k;
	for (uint64_t h = 0; h < k; h++) sum += M[i*N + (j1 + h)] * M[(i2 - h)*N + (i+k)];
	sum = std::cbrt(sum);
	//std::cout << sum << std::endl;
	M[i*N + (i+k)] = sum;
}

void tileWork(uint64_t minX, uint64_t minY, uint64_t maxX, uint64_t maxY,
	std::vector<double> &M, const uint64_t &N, uint64_t K){
	for (int i = maxX; i >= (int)minX; i--){
		for (uint64_t j = minY; j <= maxY; j++){
			//std::cout << "i = " << i << " j = " << j << std::endl;
			int k = K - (i - minX) + (j - minY);
			//std::cout << "\tWorking with i = " << i << " j = " << j << " k = " << k << " N = " << N << std::endl;
			if (k >= 1){ work((uint64_t)k, (uint64_t)i, M, N); }
		}
	}
}

void sequentialWavefront(std::vector<double> &M, const uint64_t &N) {
	for(uint64_t k = 1; k< N; ++k) {        // for each upper diagonal
		for(uint64_t i = 0; i< (N-k); ++i) {// for each elem. in the diagonal
			work(k, i, M, N);
		}
	}
}

void run(uint64_t N, uint64_t threadNum, uint64_t policy, uint64_t chunkSize,
	uint64_t tileSize, const std::string& filename){

	// Create the barrier
	std::barrier syncPoint(threadNum, [](){ });

	// allocate the matrix
	std::vector<double> M(N*N, 0.0);

	std::ofstream output_file;
	output_file.open(filename, std::ios_base::app);

	output_file << "Parameters: N = " << N << " threadNum = " << threadNum << " policy = " << policy 
		<< " tileSize = " << tileSize << " chunkSize = " << chunkSize << std::endl;

	// init function
	auto init=[&]() {
		for (uint64_t i = 0; i < N; i++){
			M[i*N + i] = (i + 1.) / (double)N;
		}
	};
	
	init();

	// Policy #1: block distribution along a (possibly tiled) diagonal
	auto blockWavefront = [&](std::vector<double> &M, const uint64_t &N, uint64_t threadId,
		uint64_t threadNum, uint64_t tileSize){
		for (uint64_t K = 0; K < N; K += tileSize) { // K <- kt
			// X_t(k, i) = [(ti, ti+kt), (ti+t-1, ti+kt+t-1)]
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			uint64_t baseBlockSize = numTiles / threadNum;
			if (baseBlockSize == 0) baseBlockSize = 1;
			uint64_t start = threadId * baseBlockSize; // start block
			uint64_t end; // end block
			if (threadId + 1 < threadNum)
				end = std::min((threadId + 1) * baseBlockSize, numTiles);
			else
				end = numTiles;
			uint64_t minX, minY, maxX, maxY;
			for (uint64_t i = start; i < end; i++) {
				// Compute coordinates
				minX = tileSize * i;
				minY = minX + K;
				maxX = std::min(minX + tileSize - 1, N);
				maxY = std::min(minY + tileSize - 1, N);
				//std::cout << "Working with minX = " << minX << " minY = " << minY <<
				//	" maxX = " << maxX << " maxY = " << maxY << " K = " << K << std::endl;
				tileWork(minX, minY, maxX, maxY, M, N, K);
			}
			syncPoint.arrive_and_wait();
		}
	};

	// Cyclic distribution policy along (possibly tiled) diagonals
	auto cyclicWavefront = [&](std::vector<double> &M, const uint64_t &N, uint64_t threadId,
		uint64_t threadNum, uint64_t tileSize){
		for (uint64_t K = 0; K < N; K += tileSize) { // K <- kt
			// X_t(k, i) = [(ti, ti+kt), (ti+t-1, ti+kt+t-1)]
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			uint64_t minX, minY, maxX, maxY;
			for (uint64_t i = threadId; i < numTiles; i += threadNum) {
				// Compute coordinates
				minX = tileSize * i;
				minY = minX + K;
				maxX = std::min(minX + tileSize - 1, N);
				maxY = std::min(minY + tileSize - 1, N);
				//std::cout << "Working with minX = " << minX << " minY = " << minY <<
				//	" maxX = " << maxX << " maxY = " << maxY << " K = " << K << std::endl;
				tileWork(minX, minY, maxX, maxY, M, N, K);
			}
			syncPoint.arrive_and_wait();
		}
	};

	// Block Cyclic distribution policy along (possibly tiled) diagonals
	auto blockCyclicWavefront = [&](std::vector<double> &M, const uint64_t &N, uint64_t threadId,
		uint64_t threadNum, uint64_t tileSize, uint64_t chunkSize){
		for (uint64_t K = 0; K < N; K += tileSize) { // K <- kt
			// X_t(k, i) = [(ti, ti+kt), (ti+t-1, ti+kt+t-1)]
			uint64_t numTiles = (N - K + tileSize - 1) / tileSize;
			uint64_t minX, minY, maxX, maxY;
			for (uint64_t j = 0; j < numTiles; j += chunkSize * threadNum){
				uint64_t start = j + threadId * chunkSize;
				uint64_t end = std::min(j + (threadId + 1) * chunkSize, numTiles);
				for (uint64_t i = start; i < end; i++) {
					// Compute coordinates
					minX = tileSize * i;
					minY = minX + K;
					maxX = std::min(minX + tileSize - 1, N);
					maxY = std::min(minY + tileSize - 1, N);
					tileWork(minX, minY, maxX, maxY, M, N, K);
				}
				syncPoint.arrive_and_wait();
			}
		}
	};

	TIMERSTART(wavefront, 1000, "ms", output_file, "Time: ");
	// create both the barrier and threads pool
	std::vector<std::thread> threads;
	std::cout << "Using " << threadNum << " threads" << std::endl;
	threads.reserve(threadNum);
	// Now spawn the threads and go
	if (policy == 0){
		sequentialWavefront(M, N);
	} else if (policy == 1){ // block policy
		for (uint64_t i = 0; i < threadNum; i++)
			threads.emplace_back(
				blockWavefront, std::ref(M), N, i, threadNum, tileSize
			);
	} else if (policy == 2){ // cyclic policy
		for (uint64_t i = 0; i < threadNum; i++)
			threads.emplace_back(
				cyclicWavefront, std::ref(M), N, i, threadNum, tileSize
			);
	} else if (policy == 3){
		for (uint64_t i = 0; i < threadNum; i++)
			threads.emplace_back(
				blockCyclicWavefront, std::ref(M), N, i, threadNum, tileSize, chunkSize
			);
	} else {
		std::cerr << "Error: invalid policy id " << policy << std::endl;
	}
	// join each thread at the end
	for (auto& thread: threads)
		thread.join();
    TIMERSTOP(wavefront, 1000, "ms", output_file, "Time: ");
	output_file << computeChecksum(M, N) << std::endl;
	std::cout << computeChecksum(M, N) << std::endl;
}


int main(int argc, char *argv[]) {
	//std::vector<uint64_t> sizes = {2000, 4000, 6000, 8000, 10000};
	//std::vector<uint64_t> threadNums = {1, 2, 4, 6, 8, 10, 12, 14, 16};
	std::vector<uint64_t> sizes = {2000};
	std::vector<uint64_t> threadNums = {1, 2, 4, 8, 16};
	std::vector<uint64_t> policies = {0, 1, 2, 3};
	std::vector<uint64_t> tileSizes = {1, 4, 8, 16};
	std::vector<uint64_t> chunkSizes = {8, 16, 32};
	std::string filename = "output_results.txt";
	if (argc > 1) filename = argv[1];
	for (auto& N : sizes){
		for (auto& threadNum : threadNums){
			for (auto& policy : policies){
				for (auto& tileSize : tileSizes){
					if (policy < 3) run(N, threadNum, policy, 1, tileSize, filename);
					else for (auto& chunkSize : chunkSizes) run(N, threadNum, policy, chunkSize, tileSize, filename);
				}
			}
		}
	}
    return 0;
}