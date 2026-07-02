#!/bin/bash
#SBATCH --partition amd-512
#SBATCH --nodes 1
#SBATCH --ntasks-per-node=4
#SBATCH --cpus-per-task=32
#SBATCH --exclusive
#SBATCH --time 04:00:00
#SBATCH --job-name kmeans-mpi-k3

ulimit -s unlimited
module load compilers/nvidia/nvhpc/24.11
export OMP_NUM_THREADS=32
export OMP_PROC_BIND=close
export OMP_PLACES=cores

mkdir -p output_logs
LOG_FILE="output_logs/mpi_k3_results.csv"
echo "experimento,k,alunos,threads,mpi_procs,tempo" > $LOG_FILE

K=3
echo "================================================="
echo "[AUDIT] PREPARANDO MPI PARA K=$K"
echo "================================================="
sed -i "s/#define NUM_CLUSTERS .*/#define NUM_CLUSTERS $K/g" KmeansOpenMPMPI.c
make clean > /dev/null
make KmeansOpenMPMPI > /dev/null

MPI_PROCS=4
THREADS=32

echo ">>> EXPERIMENTO: ESCALABILIDADE FRACA (VARIANDO ALUNOS) K=$K <<<"
for ALUNOS in 20000 40000 80000 160000 320000 640000 1000000; do
    echo "================================================="
    echo "[AUDIT] Rodando MPI Alunos=$ALUNOS, K=$K, MAX_ITER=10000, FIT=10, RANDOM_STATE=42, MPI_PROCS=$MPI_PROCS, OMP_THREADS=$THREADS"
    echo "[AUDIT] Total CPUs alocados no nó: $(($MPI_PROCS * $THREADS)) (${MPI_PROCS} processos x ${THREADS} threads)"
    mpi_time=$(mpirun -np $MPI_PROCS --map-by socket:PE=$THREADS ./KmeansOpenMPMPI $THREADS $ALUNOS 10 | tee /dev/stderr | grep "Duração do treinamento" | awk '{print $4}')
    echo "fraca,$K,$ALUNOS,$THREADS,$MPI_PROCS,$mpi_time" >> $LOG_FILE
done

echo "================================================="
echo "[AUDIT] EXECUÇÃO FINALIZADA"
echo "================================================="
