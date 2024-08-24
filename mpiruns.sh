#!/bin/bash

# Define parameter ranges or lists
N_list=(1000 2000 3000 4000 6000 8000)
policy_list=(1)
ntasks_list=(1 2 4 6 8 10 12 16 20 24 32 48 64) # Number of tasks
tileSize_list=(1 4 8 16)

read -p "Enter the number of nodes to use: " nnodes
echo "Using $nnodes nodes ..."

# Loop over all combinations of parameters
for N in "${N_list[@]}"; do
    for policy in "${policy_list[@]}"; do
        for ntasks in "${ntasks_list[@]}"; do
            if [ "$ntasks" -ge "$nnodes"]; then
                for tileSize in "${tileSize_list[@]}"; do
                    echo "Running with parameters: N=$N, nnodes=$nnodes, ntasks=$ntasks, tileSize=$tileSize"
                    srun --mpi=pmix --nodes $nnodes --ntasks $ntasks -e spmcluster_mpi_err.log ./UTWavefrontMPI $N $tileSize $policy $nnodes output_results_mpi_spmcluster.csv
                    if [ $? -ne 0 ]; then
                        echo "An error occurred with parameters: N=$N, nnodes=$nnodes, ntasks=$ntasks, tileSize=$tileSize"
                        read -p "Continue execution (y/n)?" cont
                        if [ "$cont" == "n" ]; then
                            break 4  # Only for bash 4 or above
                        fi
                    fi
                done
            fi
        done
    done
done
