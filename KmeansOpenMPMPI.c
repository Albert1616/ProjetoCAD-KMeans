#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>
#include "Model/aluno.h"
#include "Model/dataset.h"
#include "Model/kmeans.h"

#define NUM_CLUSTERS 3
#define MAX_ITERATIONS 10000
#define NUM_ALUNOS 200000
#define NUM_FIT_ITERATIONS 50

// Protótipos das funções no utils hybrid (não estão no kmeans.h original)
void fitHybrid(KMeans *model, Aluno *alunosLocal, Aluno *todosAlunos, int localSize, int rank, int numIteracoes);

void exportarResultadosHybrid(Aluno *alunos, int total)
{
    FILE *file = fopen("resultados_hybrid.csv", "w");
    fprintf(file, "numeroFaltas,media,cluster\n");

    for (int i = 0; i < total; i++)
        fprintf(file, "%.2f,%.2f,%d\n",
                alunos[i].numeroFaltas,
                alunos[i].media,
                alunos[i].cluster);

    fclose(file);
}

int main(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double inicio, fim;
    double tempoLocal, tempoTotal;
    int totalAlunos = 0;
    Aluno *todosAlunos = NULL;

    if (argc > 1)
    {
        int nt = atoi(argv[1]);
        if (nt > 0)
            omp_set_num_threads(nt);
    }

    // Usuário pode definir número de threads em tempo de execução
    printf("MPI Size: %d processes, OpenMP Threads per process: %d\n", size, omp_get_max_threads());

    if (rank == 0)
    {
        todosAlunos = (Aluno *)malloc(NUM_ALUNOS * sizeof(Aluno)); // Buffer maior para segurança
        totalAlunos = carregarDataset(todosAlunos);
        normalizarAlunos(todosAlunos, totalAlunos);
    }

    // Compartilha o total de alunos para que todos possam calcular seus tamanhos locais
    MPI_Bcast(&totalAlunos, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular quantos alunos cada processo vai receber
    int *sendcounts_alunos = (int *)malloc(size * sizeof(int));
    int *sendcounts_bytes = (int *)malloc(size * sizeof(int));
    int *displs_bytes = (int *)malloc(size * sizeof(int));

    int rem = totalAlunos % size;
    int offset = 0;
    for (int i = 0; i < size; i++)
    {
        sendcounts_alunos[i] = totalAlunos / size + (i < rem ? 1 : 0);
        sendcounts_bytes[i] = sendcounts_alunos[i] * sizeof(Aluno);
        displs_bytes[i] = offset * sizeof(Aluno);
        offset += sendcounts_alunos[i];
    }

    int localSize = sendcounts_alunos[rank];
    Aluno *alunosLocal = (Aluno *)malloc(localSize * sizeof(Aluno));

    // Distribui os dados usando contagem de bytes
    MPI_Scatterv(todosAlunos, sendcounts_bytes, displs_bytes, MPI_BYTE, alunosLocal, localSize * sizeof(Aluno), MPI_BYTE, 0, MPI_COMM_WORLD);

    // Configuração do modelo
    KMeans kmeans;
    kmeans.k = NUM_CLUSTERS;
    kmeans.max_iter = MAX_ITERATIONS;
    kmeans.random_state = 42;
    kmeans.centroids = (Aluno *)malloc(kmeans.k * sizeof(Aluno));
    kmeans.totalAlunos = totalAlunos; // Referência global para inicialização

    // Treinamento Híbrido
    inicio = MPI_Wtime();

    fitHybrid(&kmeans, alunosLocal, todosAlunos, localSize, rank, NUM_FIT_ITERATIONS);

    fim = MPI_Wtime();
    tempoLocal = fim - inicio;

    MPI_Reduce(&tempoLocal, &tempoTotal, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Coleta resultados de volta no Rank 0 para exportação usando contagem de bytes
    MPI_Gatherv(alunosLocal, localSize * sizeof(Aluno), MPI_BYTE, todosAlunos, sendcounts_bytes, displs_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        // exportarResultadosHybrid(todosAlunos, totalAlunos);
        // printf("Resultados exportados para resultados_hybrid.csv\n");

        // Visualizar centroides
        for (int i = 0; i < kmeans.k; i++)
        {
            printf("Centroid %d - Media: %.2f, Numero de Faltas: %.2f\n", i, kmeans.centroids[i].media, kmeans.centroids[i].numeroFaltas);
        }

        printf("Tempo de execução: %.2f segundos.", tempoTotal);
    }

    free(kmeans.centroids);
    free(alunosLocal);
    free(sendcounts_alunos);
    free(sendcounts_bytes);
    free(displs_bytes);
    if (rank == 0)
        free(todosAlunos);

    MPI_Finalize();
    return 0;
}
