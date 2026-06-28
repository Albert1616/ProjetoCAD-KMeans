#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <mpi.h>
#include "../Model/aluno.h"
#include "../Model/kmeans.h"

// Utilitários compartilhados
float distEuclidiana(Aluno *aluno, Aluno *centroid)
{
    float diffMedia = pow(aluno->media - centroid->media, 2);
    float diffFaltas = pow(aluno->numeroFaltas - centroid->numeroFaltas, 2);

    return sqrt(diffMedia + diffFaltas);
}

int minListaIndex(float lista[], int tamanho)
{
    float min = lista[0];
    int index = 0;

    for (int i = 0; i < tamanho; i++)
    {
        if (lista[i] < min)
        {
            index = i;
            min = lista[i];
        }
    }

    return index;
}

/**
 * O rank 0 escolhe os índices aleatórios e transmite para todos.
 * Justificativa: Garante que todos os processos comecem com os mesmos centroides sem redundância de cálculo.
 */
void initCentroidsMPI(KMeans *model, Aluno *alunos, int rank)
{
    if (rank == 0)
    {
        int listIndex[model->k];
        for (int i = 0; i < model->k; i++)
            listIndex[i] = -1;

        unsigned int seed = model->random_state;
        for (int i = 0; i < model->k; i++)
        {
            // Nota: totalAlunos aqui deve ser o total GLOBAL para a inicialização correta
            // Mas como o rank 0 tem acesso ao dataset completo antes do scatter, usamos isso.
            int index = rand_r(&seed) % model->totalAlunos;
            int duplicado = 0;

            for (int j = 0; j < i; j++)
            {
                if (listIndex[j] == index)
                {
                    duplicado = 1;
                    break;
                }
            }

            if (duplicado)
            {
                i--;
                continue;
            }

            listIndex[i] = index;
            model->centroids[i] = alunos[index];
        }
    }

    // Define tipo MPI para Aluno se necessário, ou envia como bytes se a estrutura for simples.
    // Para simplicidade e performance em structs pequenas sem ponteiros, enviamos como bytes.
    MPI_Bcast(model->centroids, model->k * sizeof(Aluno), MPI_BYTE, 0, MPI_COMM_WORLD);
}

// Atribui cada aluno ao cluster mais próximo usando OpenMP para paralelismo local.
void assignClustersHybrid(KMeans *model, Aluno *alunosLocal, int localSize)
{
#pragma omp parallel for schedule(static)
    for (int i = 0; i < localSize; i++)
    {
        float distanciaClusters_local[model->k];

        for (int j = 0; j < model->k; j++)
        {
            distanciaClusters_local[j] = distEuclidiana(&alunosLocal[i], &model->centroids[j]);
        }

        alunosLocal[i].cluster = minListaIndex(distanciaClusters_local, model->k);
    }
}

/**
 * Atualiza os centroides agregando somas locais (OpenMP) e globais (MPI_Allreduce).
 * Justificativa: O uso de buffers locais evita condições de corrida em OpenMP,
 * enquanto o Allreduce sincroniza os resultados entre nós de processamento.
 */
void updateCentroidsHybrid(KMeans *model, Aluno *alunosLocal, int localSize)
{
    float *local_sum_media = (float *)calloc(model->k, sizeof(float));
    float *local_sum_faltas = (float *)calloc(model->k, sizeof(float));
    int *local_count = (int *)calloc(model->k, sizeof(int));

// Passo 1: Soma local com OpenMP
#pragma omp parallel
    {
        float thread_sum_media[model->k];
        float thread_sum_faltas[model->k];
        int thread_count[model->k];

        for (int c = 0; c < model->k; c++)
        {
            thread_sum_media[c] = 0.0f;
            thread_sum_faltas[c] = 0.0f;
            thread_count[c] = 0;
        }

#pragma omp for nowait
        for (int i = 0; i < localSize; i++)
        {
            int clusterIndex = alunosLocal[i].cluster;
            thread_sum_media[clusterIndex] += alunosLocal[i].media;
            thread_sum_faltas[clusterIndex] += alunosLocal[i].numeroFaltas;
            thread_count[clusterIndex]++;
        }

        // Redução crítica para os buffers locais do processo
        for (int c = 0; c < model->k; c++)
        {
#pragma omp atomic
            local_sum_media[c] += thread_sum_media[c];
#pragma omp atomic
            local_sum_faltas[c] += thread_sum_faltas[c];
#pragma omp atomic
            local_count[c] += thread_count[c];
        }

        free(thread_sum_media);
        free(thread_sum_faltas);
        free(thread_count);
    }

    // Passo 2: Agregação global com MPI
    float *global_sum_media = (float *)malloc(model->k * sizeof(float));
    float *global_sum_faltas = (float *)malloc(model->k * sizeof(float));
    int *global_count = (int *)malloc(model->k * sizeof(int));

    MPI_Allreduce(local_sum_media, global_sum_media, model->k, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(local_sum_faltas, global_sum_faltas, model->k, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(local_count, global_count, model->k, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    // Passo 3: Atualizar centroides em todos os processos
    for (int c = 0; c < model->k; c++)
    {
        if (global_count[c] > 0)
        {
            model->centroids[c].media = global_sum_media[c] / global_count[c];
            model->centroids[c].numeroFaltas = global_sum_faltas[c] / global_count[c];
        }
    }

    free(local_sum_media);
    free(local_sum_faltas);
    free(local_count);
    free(global_sum_media);
    free(global_sum_faltas);
    free(global_count);
}

void fitHybrid(KMeans *model, Aluno *alunosLocal, Aluno *todosAlunos, int localSize, int rank, int numIteracoes)
{
    for (int iter = 0; iter < numIteracoes; iter++)
    {
        initCentroidsMPI(model, todosAlunos, rank);
        for (int i = 0; i < model->max_iter; i++)
        {
            Aluno old_centroids[model->k];
            for (int j = 0; j < model->k; j++)
                old_centroids[j] = model->centroids[j];

            assignClustersHybrid(model, alunosLocal, localSize);
            updateCentroidsHybrid(model, alunosLocal, localSize);

            int convergiu_local = 1;
            for (int j = 0; j < model->k; j++)
            {
                if (distEuclidiana(&model->centroids[j], &old_centroids[j]) > 0.001)
                {
                    convergiu_local = 0;
                    break;
                }
            }

            int convergiu_global;
            MPI_Allreduce(&convergiu_local, &convergiu_global, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

            if (convergiu_global)
            {
                break;
            }
        }
    }
}
