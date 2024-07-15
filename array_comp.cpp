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
    if (argc<4) {
        std::cerr << "use: " << argv[0]  << " number1 number2 nworkers [chunk=0] [print=off|on]\n";
        return -1;
    }
    std::mutex mutex;
    ull n1          = std::stoll(argv[1]);
    ull n2          = std::stoll(argv[2]);  
    size_t nworkers = std::stol(argv[3]);  
    long chunk = 0;
    if (argc >= 5)  chunk = std::stol(argv[4]);
    bool print_primes = false;
    if (argc >= 6)  print_primes = (std::string(argv[5]) == "on");
    std::vector<ull> results((size_t)(n2-n1), 0);
    std::vector<size_t> count(nworkers, 0);
    std::vector<long> doers((size_t)(n2-n1), -1);
    std::cout << std::endl;

    ffTime(START_TIME);
    parallel_for_idx(n1, n2, 1, chunk, [&](const long start, const long stop, const long thid){
        for (long i = start; i < stop; i++){
            ull sum = 0;
            for (long j = 0; j < i; j++) sum += (ull)j;
            results[i-n1] = sum;
            count[thid] =  count[thid] + 1;
            doers[i-n1] = thid;
        }
    }, nworkers);
    ffTime(STOP_TIME);

    if (print_primes) {
        printVector(results, NULL);
        printVector(count, NULL);
        printVector(doers, NULL);
    }
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
}