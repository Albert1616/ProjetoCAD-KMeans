CC = gcc
CFLAGS = -O2 -Wall -IModel
LDFLAGS = -lm -fopenmp

# Alvos
SEQ_TARGET = KmeansSequencial
OMP_TARGET = KmeansOpenMP

.PHONY: all clean run-seq run-omp help

all: $(SEQ_TARGET) $(OMP_TARGET)

# Versão sequencial
$(SEQ_TARGET): KmeansSequencial.c Util/Kmeans.c Util/CarregarDataset.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Versão OpenMP
$(OMP_TARGET): KmeansOpenMP.c Util/KmeansOpenMP_utils.c Util/CarregarDataset.c
	$(CC) $(CFLAGS) -fopenmp -o $@ $^ $(LDFLAGS)

# Execução
run-seq: $(SEQ_TARGET)
	./$(SEQ_TARGET)

run-omp: $(OMP_TARGET)
	./$(OMP_TARGET) $$(nproc)

clean:
	rm -f $(SEQ_TARGET) $(OMP_TARGET) resultados.csv *.o Util/*.o

help:
	@echo "Alvos disponíveis:"
	@echo "  make all       - Compila ambas versões (sequencial e OpenMP)"
	@echo "  make $(SEQ_TARGET)  - Compila versão sequencial"
	@echo "  make $(OMP_TARGET) - Compila versão OpenMP"
	@echo "  make run-seq   - Compila e roda versão sequencial"
	@echo "  make run-omp   - Compila e roda versão OpenMP com nproc threads"
	@echo "  make clean     - Remove binários e arquivos temporários"
	@echo ""
	@echo "Exemplos:"
	@echo "  make run-seq                  # Rodar versão sequencial"
	@echo "  make run-omp                  # Rodar versão OpenMP com todos os cores"
	@echo "  ./$(OMP_TARGET) 4             # Rodar versão OpenMP manualmente com 4 threads"
