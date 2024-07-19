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

int main(int argc, char* argv[]){
    // Help message
    if (argc < 2){
        std::cerr << "Use: " << argv[0] << " N T G" << std::endl;
        std::cerr << "\tN = vector size" << std::endl;
        std::cerr << "\tT = number of threads" << std::endl;
        std::cerr << "\tG = grain of the parallel_for partition" << std::endl;
    }
    ull N = std::stoll(argv[1]);
    ull T = std::stoll(argv[2]);
    ull G = std::stoll(argv[3]);
    std::vector<ull> x((size_t)N, 0);
    std::vector<ull> y((size_t)N, 0);
    for (ull i = 0; i < N; i++){
        x[i] = i;
        y[i] = i;
    }
    std::vector<ull> z((size_t)N, 0);
    ParallelFor loop(16);
    ffTime(START_TIME);
    loop.parallel_for(0, N, 1, G, [&](const ull i){
        z[i] = x[i] * y[i];
    }, T);
    ffTime(STOP_TIME);
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
    //printVector(z, NULL);
    return 0;
}