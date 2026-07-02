#!/bin/bash
#SBATCH --partition gpu-8-v100
#SBATCH --nodes 1
#SBATCH --gpus-per-node=1
#SBATCH --time 00:30:00
#SBATCH --job-name kmeans-gpu-k3

ulimit -s unlimited
module load compilers/nvidia/nvhpc/24.11

mkdir -p output_logs
LOG_FILE="output_logs/gpu_k3_results.csv"
echo "experimento,k,alunos,threads,tempo" > $LOG_FILE

K=3
echo "================================================="
echo "[AUDIT] PREPARANDO GPU PARA K=$K"
echo "================================================="
sed -i "s/#define NUM_CLUSTERS .*/#define NUM_CLUSTERS $K/g" KmeansSequencial.c
make clean > /dev/null
make KmeansOpenMPGPU > /dev/null

export OMP_THREAD_LIMIT=32

echo ">>> EXPERIMENTO: ESCALABILIDADE FRACA (VARIANDO ALUNOS) K=$K <<<"
for ALUNOS in 20000 40000 80000 160000 320000 640000 1000000; do
    echo "================================================="
    echo "[AUDIT] Rodando GPU Alunos=$ALUNOS, K=$K, MAX_ITER=10000, FIT=10, RANDOM_STATE=42, OMP_THREAD_LIMIT=32"
    echo "================================================="
    gpu_time=$(./KmeansOpenMPGPU $ALUNOS 10 | tee /dev/stderr | grep "Duração do treinamento" | awk '{print $4}')
    echo "fraca,$K,$ALUNOS,32,$gpu_time" >> $LOG_FILE
done

echo "================================================="
echo "[AUDIT] EXECUÇÃO FINALIZADA"
echo "================================================="
