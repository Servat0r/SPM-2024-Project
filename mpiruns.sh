#!/bin/bash

# Define parameter ranges or lists
policy_list=(1)
ntasks_list=(2 3 5 9 11 13 17 21 25 33 49 65) # Number of tasks + frontend task
tileSize_list=(1 4 8 16)

read -p "Enter the number of nodes to use: " nnodes
echo "Using $nnodes nodes ..."

read -p "Enter the size of the matrix to use: " N
echo "Using a matrix of size $N ..."

# Loop over all combinations of parameters
for policy in "${policy_list[@]}"; do
    for ntasks in "${ntasks_list[@]}"; do
        if [ "$ntasks" -ge "$nnodes" ]; then
            if (( ntasks <= 16 * nnodes + 1)); then
                for tileSize in "${tileSize_list[@]}"; do
                    echo "Running with parameters: N=$N, nnodes=$nnodes, ntasks=$ntasks, tileSize=$tileSize"
                    srun --mpi=pmix --nodes $nnodes --ntasks $ntasks -e spmcluster_mpi_err.log ./UTWavefrontMPI $N $tileSize $policy $nnodes output_results_mpi_spmcluster_${N}size_${nnodes}nodes.csv
                    if [ $? -ne 0 ]; then
                        echo "An error occurred with parameters: N=$N, nnodes=$nnodes, ntasks=$ntasks, tileSize=$tileSize"
                        read -p "Continue execution (y/n)?" cont
                        if [ "$cont" == "n" ]; then
                            break 3  # Only for bash 4 or above
                        fi
                    fi
                done
            fi
        fi
    done
done
