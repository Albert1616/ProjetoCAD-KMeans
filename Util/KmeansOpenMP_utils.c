#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "../Model/aluno.h"
#include "../Model/kmeans.h"

// Utilitários para o KMeans
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

void initCentroids(KMeans *model, Aluno *alunos)
{
    int listIndex[model->k];
    for (int i = 0; i < model->k; i++)
        listIndex[i] = -1;

    // Serial geração de índices com rand_r() thread-safe.
    // Não paralelizamos essa parte pois requer sincronização complexa para evitar duplicatas.
    unsigned int seed = model->random_state;
    for (int i = 0; i < model->k; i++)
    {
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
    }

    // Paraleliza cópia dos centroids: cada thread copia seus índices.
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < model->k; i++)
    {
        model->centroids[i] = alunos[listIndex[i]];
    }
}

void assignClusters(KMeans *model, Aluno *alunos)
{
    // Paraleliza o loop sobre alunos. Cada iteração usa um array local
    // 'distanciaClusters_local' para evitar condições de corrida.
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < model->totalAlunos; i++)
    {
        float distanciaClusters_local[model->k];

        for (int j = 0; j < model->k; j++)
        {
            distanciaClusters_local[j] = distEuclidiana(&alunos[i], &model->centroids[j]);
        }

        alunos[i].cluster = minListaIndex(distanciaClusters_local, model->k);
    }
}

void updateCentroids(KMeans *model, Aluno *alunos)
{
    // Usamos buffers por thread para somas e contagens para evitar atomics
    int nthreads = omp_get_max_threads();
    float *sum_media = (float *)calloc(nthreads * model->k, sizeof(float));
    float *sum_faltas = (float *)calloc(nthreads * model->k, sizeof(float));
    int *count = (int *)calloc(nthreads * model->k, sizeof(int));

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int base = tid * model->k;

        #pragma omp for nowait
        for (int i = 0; i < model->totalAlunos; i++)
        {
            int clusterIndex = alunos[i].cluster;
            int idx = base + clusterIndex;
            sum_media[idx] += alunos[i].media;
            sum_faltas[idx] += alunos[i].numeroFaltas;
            count[idx]++;
        }
    }

    // Agregar resultados por cluster
    for (int c = 0; c < model->k; c++)
    {
        float media_sum = 0.0f;
        float faltas_sum = 0.0f;
        int total_count = 0;

        for (int t = 0; t < nthreads; t++)
        {
            int idx = t * model->k + c;
            media_sum += sum_media[idx];
            faltas_sum += sum_faltas[idx];
            total_count += count[idx];
        }

        if (total_count > 0)
        {
            model->centroids[c].media = media_sum / total_count;
            model->centroids[c].numeroFaltas = faltas_sum / total_count;
        }
    }

    free(sum_media);
    free(sum_faltas);
    free(count);
}

void fit(KMeans *model, Aluno *alunos)
{
    double t_start = omp_get_wtime();

    initCentroids(model, alunos);

    for (int i = 0; i < model->max_iter; i++)
    {
        Aluno old_centroids[model->k];
        for (int j = 0; j < model->k; j++)
            old_centroids[j] = model->centroids[j];

        assignClusters(model, alunos);
        updateCentroids(model, alunos);
        int convergiu = 1;

        for (int j = 0; j < model->k; j++)
        {
            if (distEuclidiana(&model->centroids[j], &old_centroids[j]) > 0.01)
            {
                convergiu = 0;
                break;
            }
        }

        if (convergiu)
        {
            break;
        }
    }

    double t_end = omp_get_wtime();
    printf("KMeans fit wall time: %.6f s\n", t_end - t_start);
}

void predict(KMeans *model, Aluno *novoAluno)
{
    float distanciaClusters[model->k];

    for (int i = 0; i < model->k; i++)
    {
        distanciaClusters[i] = distEuclidiana(novoAluno, &model->centroids[i]);
    }

    novoAluno->cluster = minListaIndex(distanciaClusters, model->k);
}

float *methodElbow(KMeans *model, Aluno *alunos)
{
    int k_range[] = {2, 3, 4, 5, 6, 7, 8};
    int n = 7;
    float *inertia_values = (float *)malloc(n * sizeof(float));

    // Paraleliza iteração sobre valores de k. Cada thread recebe sua própria cópia
    // do modelo KMeans para evitar race conditions no estado dos centroids.
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        // Cópia privada do modelo para essa thread
        KMeans model_local = *model;
        model_local.k = k_range[i];
        model_local.centroids = (Aluno *)malloc(model_local.k * sizeof(Aluno));

        // Treina com k local
        fit(&model_local, alunos);

        // Calcula inércia
        float inertia = 0;
        for (int j = 0; j < model_local.totalAlunos; j++)
            inertia += pow(distEuclidiana(&alunos[j], &model_local.centroids[alunos[j].cluster]), 2);

        inertia_values[i] = inertia;
        free(model_local.centroids);
    }
    return inertia_values;
}