#ifndef KMEANS_H
#define KMEANS_H
#include "aluno.h"

typedef struct
{
    int k;
    int max_iter;
    int random_state;
    int totalAlunos;
    Aluno *centroids;
} KMeans;

void initCentroids(KMeans *model, Aluno *alunos);
void assignClusters(KMeans *model, Aluno *alunos);
void updateCentroids(KMeans *model, Aluno *alunos);
void predict(KMeans *model, Aluno *novoAluno);
void fit(KMeans *model, Aluno *alunos, int numIteracoes);
float *methodElbow(KMeans *model, Aluno *alunos, int numIteracoes);

#endif // KMEANS_H