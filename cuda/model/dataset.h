#ifndef DATASET_H
#define DATASET_H
#include "aluno.h"

#ifdef __CUDACC__
__host__ int carregarDataset(Aluno *alunos, int numAlunos);
__host__ void normalizarAlunos(Aluno *alunos, int total);
#endif

#endif // DATASET_H
