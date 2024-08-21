#!/bin/bash

# Define parameter ranges or lists
N_list=(12000)
policy_list=(1)
nworkers_list=(1)
tileSize_list=(8 16 32)

# Sequential Runs
for N in "${N_list[@]}"; do
    for tileSize in "${tileSize_list[@]}"; do
        echo "Sequential run with parameters: N=$N, nworkers=1, tileSize=$tileSize"
        mpirun -np 1 ./UTWBPWMPI $N $tileSize 0
    done
done

# Loop over all combinations of parameters
for N in "${N_list[@]}"; do
    for policy in "${policy_list[@]}"; do
        for nworkers in "${nworkers_list[@]}"; do
            for tileSize in "${tileSize_list[@]}"; do
                echo "Running with parameters: N=$N, nworkers=$nworkers, tileSize=$tileSize"
                mpirun -np $nworkers ./UTWBPWMPI $N $tileSize $policy
            done
        done
    done
done
