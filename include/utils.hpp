#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
//#include <openssl/sha.h>

void displayMatrix(std::vector<double> &M, const uint64_t &N){
	for (uint64_t i = 0; i < N; i++){
		for (uint64_t j = 0; j < N; j++)
			std::printf("%.4f  ", M[i*N+j]);
		std::cout << std::endl;
	}
}

void writeMatrixToFile(std::vector<double> &M, const uint64_t &N, const std::string& filename){
    std::ofstream output_file(filename, std::ios::binary);
    if (!output_file) {
            throw std::runtime_error("Failed to open file for writing");
    }

    output_file.write(reinterpret_cast<const char*>(&N), sizeof(N));
    for (uint64_t i = 0; i < N*N; i++){
        output_file.write(reinterpret_cast<const char*>(&M[i]), sizeof(double));
    }
    
    output_file.close();
}

std::vector<double> readMatrixFromFile(const std::string& filename) {
    std::ifstream input_file(filename, std::ios::binary);
    if (!input_file) {
        throw std::runtime_error("Failed to open file for reading");
    }

    // Read the size of the matrix
    uint64_t size;
    input_file.read(reinterpret_cast<char*>(&size), sizeof(size));

    // Read the matrix data
    std::vector<double> matrix(size*size, 0.0);
    for (auto& row : matrix) {
        input_file.read(reinterpret_cast<char*>(&row), sizeof(row));
    }

    input_file.close();
    return matrix;
}

uint64_t computeChecksum(std::vector<double>& M, uint64_t& N){
    std::vector<uint64_t> results(N, 0);
    for (uint64_t i = 0; i < N; i++){
        uint64_t result = 0;
        for (uint64_t j = 0; j < N; j++)
            result = result ^ (uint64_t)M[i*N + j];
        results[i] = result;
    }
    uint64_t final = 0;
    for (uint64_t i = 0; i < N; i++) final = final + results[i];
    return final;
}

bool compare_files(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary | std::ios::ate);
    std::ifstream f2(file2, std::ios::binary | std::ios::ate);

    if (!f1.is_open() || !f2.is_open()) {
        throw std::runtime_error("Failed to open one or both files.");
    }

    // Compare file sizes
    std::streamsize size1 = f1.tellg();
    std::streamsize size2 = f2.tellg();
    if (size1 != size2) {
        return false;
    }

    // Reset file read positions to the beginning
    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);

    // Compare contents byte by byte
    char bytes1, bytes2;
    while (f1.read(&bytes1, sizeof(char)) && f2.read(&bytes2, sizeof(char))) {
        if (bytes1 != bytes2) {
            return false;
        }
    }

    return true;
}

#endif