#!/bin/bash

#SBATCH --partition gpu-8-v100

#SBATCH --gpus-per-node=1

#SBATCH --nodes 1

#SBATCH --time 00:10:00

#SBATCH --job-name kmeans

#SBATCH --output kmeans-%j.out


ulimit -s unlimited


module load compilers/nvidia/cuda/12.6


set -e

nvcc -arch=sm_70 -O3 main.cu kmeans.cu dataset.cu -o cuda_kmeans

nsys profile --stats=true --trace=cuda,osrt --force-overwrite=true -o profile_kmeans ./cuda_kmeans

rm cuda_kmeans
