#include <math.h>
#include <stdlib.h>
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

    srand(model->random_state);

    for (int i = 0; i < model->k; i++)
    {
        int index = rand() % model->totalAlunos;
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

void assignClusters(KMeans *model, Aluno *alunos)
{
    float distanciaClusters[model->k];

    for (int i = 0; i < model->totalAlunos; i++)
    {
        for (int j = 0; j < model->k; j++)
        {
            float distancia = distEuclidiana(&alunos[i], &model->centroids[j]);
            distanciaClusters[j] = distancia;
        }

        alunos[i].cluster = minListaIndex(distanciaClusters, model->k);
    }
}

void updateCentroids(KMeans *model, Aluno *alunos)
{
    Aluno newCentroids[model->k];
    int countAlunos[model->k];

    for (int i = 0; i < model->k; i++)
    {
        newCentroids[i].media = 0;
        newCentroids[i].numeroFaltas = 0;
        countAlunos[i] = 0;
    }

    for (int i = 0; i < model->totalAlunos; i++)
    {
        int clusterIndex = alunos[i].cluster;

        newCentroids[clusterIndex].media += alunos[i].media;
        newCentroids[clusterIndex].numeroFaltas += alunos[i].numeroFaltas;
        countAlunos[clusterIndex]++;
    }

    for (int i = 0; i < model->k; i++)
    {
        if (countAlunos[i] > 0)
        {
            model->centroids[i].media = newCentroids[i].media / countAlunos[i];
            model->centroids[i].numeroFaltas = newCentroids[i].numeroFaltas / countAlunos[i];
        }
    }
}

void fit(KMeans *model, Aluno *alunos, int numIteracoes)
{
    for (int iter = 0; iter < numIteracoes; iter++)
    {
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
    }
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

float *methodElbow(KMeans *model, Aluno *alunos, int numIteracoes)
{
    int k_range[] = {2, 3, 4, 5, 6, 7, 8};
    int n = 7;
    float *inertia_values = (float *)malloc(n * sizeof(float));

    for (int i = 0; i < n; i++)
    {
        model->k = k_range[i];
        free(model->centroids);
        model->centroids = (Aluno *)malloc(model->k * sizeof(Aluno));

        fit(model, alunos, numIteracoes);

        float inertia = 0;
        for (int j = 0; j < model->totalAlunos; j++)
            inertia += pow(distEuclidiana(&alunos[j], &model->centroids[alunos[j].cluster]), 2);

        inertia_values[i] = inertia;
    }
    return inertia_values;
}