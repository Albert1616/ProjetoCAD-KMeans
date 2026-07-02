CC = gcc
MPICC = mpicc
NCC = nvc
NFLAGS = -O2 -mp=gpu -Wall -IModel -D_OPENMP_GPU
CFLAGS = -O2 -Wall -IModel
LDFLAGS = -lm -fopenmp

# Alvos
SEQ_TARGET = KmeansSequencial
OMP_TARGET = KmeansOpenMP
HYB_TARGET = KmeansOpenMPMPI
OMP_GPU_TARGET = KmeansOpenMPGPU

.PHONY: all clean run-seq run-omp run-omp-mpi run-omp-gpu test-cuda help

all: $(SEQ_TARGET) $(OMP_TARGET) $(HYB_TARGET)

# Versão sequencial
$(SEQ_TARGET): KmeansSequencial.c Util/Kmeans.c Util/CarregarDataset.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Versão OpenMP
$(OMP_TARGET): KmeansOpenMP.c Util/KmeansOpenMP_utils.c Util/CarregarDataset.c
	$(CC) $(CFLAGS) -fopenmp -o $@ $^ $(LDFLAGS)

# Versão Híbrida MPI + OpenMP
$(HYB_TARGET): KmeansOpenMPMPI.c Util/KmeansOpenMPMPI_utils.c Util/CarregarDataset.c
	$(MPICC) $(CFLAGS) -fopenmp -o $@ $^ $(LDFLAGS)

# Versao OpenMP GPU
$(OMP_GPU_TARGET): KmeansSequencial.c Util/KmeansOpenMPGPU_utils.c Util/CarregarDataset.c
	$(NCC) $(NFLAGS) -o $@ $^

# Execução
run-seq: $(SEQ_TARGET)
	./$(SEQ_TARGET)

run-omp: $(OMP_TARGET)
	./$(OMP_TARGET) $$(nproc)

run-omp-mpi: $(HYB_TARGET)
	mpirun -np 2 ./$(HYB_TARGET) $$(nproc)

run-omp-gpu: $(OMP_GPU_TARGET)
	./$(OMP_GPU_TARGET)

#teste pro npad
test-cuda:
	sbatch Scripts/cuda_sbatch.sh

clean:
	rm -f $(SEQ_TARGET) $(OMP_TARGET) $(HYB_TARGET)  $(OMP_GPU_TARGET) resultados*.csv *.o Util/*.o

help:
	@echo "Alvos disponíveis:"
	@echo "  make all           - Compila todas as versões (sequencial, OpenMP e Híbrida)"
	@echo "  make $(SEQ_TARGET)      - Compila versão sequencial"
	@echo "  make $(OMP_TARGET)     - Compila versão OpenMP"
	@echo "  make $(HYB_TARGET) - Compila versão híbrida MPI+OpenMP"
	@echo "  make run-seq       - Compila e roda versão sequencial"
	@echo "  make run-omp       - Compila e roda versão OpenMP com nproc threads"
	@echo "  make run-omp-mpi   - Compila e roda versão híbrida com 2 processos e nproc threads"
	@echo "  make run-omp-gpu   - Compila e roda versão OpenMP na GPU"
	@echo "  make clean         - Remove binários e arquivos temporários"

	@echo ""
	@echo "Exemplos:"
	@echo "  make run-seq                  # Rodar versão sequencial"
	@echo "  make run-omp                  # Rodar versão OpenMP com todos os cores"
	@echo "  ./$(OMP_TARGET) 4             # Rodar versão OpenMP manualmente com 4 threads"
