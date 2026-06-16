# Projeto KMeans C

Projeto em C que aplica KMeans para agrupar alunos por média e número de faltas, desenvolvido para a disciplina de Computação de Alto Desempenho.

## Pré-requisitos
- GCC instalado
- MPI (OpenMPI ou MPICH) instalado
- `make` disponível

## Estrutura principal
- `KmeansSequencial.c` — Versão sequencial.
- `KmeansOpenMP.c` — Versão paralelizada com OpenMP (Memória Compartilhada).
- `KmeansOpenMPMPI.c` — Versão OpenMP+MPI (Memória Distribuída + Compartilhada).
- `Util/Kmeans.c` — Utilitários da versão sequencial.
- `Util/KmeansOpenMP_utils.c` — Utilitários da versão OpenMP.
- `Util/KmeansOpenMPMPI_utils.c` — Utilitários da versão OpenMP+MPI.
- `Util/CarregarDataset.c` — Carregamento e normalização do dataset.
- `Makefile` — Compila todas as versões e oferece alvos de execução.

## Compilação usando o Makefile
Navegue até o diretório do projeto e execute:

```bash
make all
```

Isso criará os binários:
- `KmeansSequencial`
- `KmeansOpenMP`
- `KmeansOpenMPMPI`

### Compilar apenas uma versão
```bash
make KmeansSequencial
make KmeansOpenMP
make KmeansOpenMPMPI
```

## Executar
### Versão sequencial
```bash
make run-seq
```

### Versão OpenMP
```bash
make run-omp
# Ou manualmente definindo threads:
./KmeansOpenMP 4
```

### Versão OpenMP+MPI
A versão OpenMP+MPI utiliza MPI para distribuir os dados entre processos e OpenMP para paralelizar o processamento dentro de cada processo.

```bash
make run-omp-mpi
# Ou manualmente (ex: 2 processos, 4 threads cada):
mpirun -np 2 ./KmeansOpenMPMPI 4
```

## Arquivo de dados
O programa espera encontrar o dataset em:
`Data/Student_performance_data.csv`

## Saída
A execução gera arquivos de resultados:
- `resultados_openmp.csv` (Versão OMP)
- `resultados_hybrid.csv` (Versão OpenMP+MPI)

## Análise de Desempenho
A versão OpenMP+MPI foi projetada para avaliar métricas como:
- **Speedup e Eficiência**: Comparando com a versão sequencial.
- **Escalabilidade Forte**: Aumento de processos/threads para um dataset fixo.
- **Escalabilidade Fraca**: Aumento proporcional de dados e poder computacional.
- **Overhead de Comunicação**: Impacto das trocas de mensagens MPI em relação ao processamento local.
