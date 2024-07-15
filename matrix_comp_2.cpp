#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include <mutex>
#include "utils.hpp"

using namespace ff;

using ull = unsigned long long;

int main(int argc, char *argv[]){
    if (argc<3) {
        std::cerr << "use: " << argv[0]  << " R C nworkers [chunk=0] [print=off|on]\n";
        return -1;
    }
    std::mutex mutex;
    size_t R = (size_t)std::stol(argv[1]);
    size_t C = (size_t)std::stol(argv[2]);
    size_t nworkers = std::stol(argv[3]);  
    long chunk = 0;
    if (argc >= 5)  chunk = std::stol(argv[4]);
    bool print_primes = false;
    if (argc >= 6)  print_primes = (std::string(argv[5]) == "on");
    std::vector<std::vector<ull>> results;
    results.reserve(R);
    for (size_t i = 0; i < R; i++){
        std::vector<ull> temp(C, 1);
        results[i] = temp;
    }
    std::vector<size_t> count(nworkers, 0);
    if (print_primes) display2DMatrix(results, R);
    std::cout << std::endl;

    ffTime(START_TIME);
    parallel_for_idx(0, R, 1, chunk, [&](const long start, const long stop, const long thid){
        for (long i = start; i < stop; i++){
            for (size_t j = 0; j < C; j++){
                ull sum = 0;
                for (ull w = 1; w <= (ull)j; w++) sum += w;
                results[(size_t)i][j] = sum;                
            }
            count[thid] =  count[thid] + 1;
        }
    }, nworkers);
    ffTime(STOP_TIME);

    if (print_primes) {
        display2DMatrix(results, R);
        std::cout << std::endl;
        printVector(count, NULL);
    }
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
}