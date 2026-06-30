#include "../Model/aluno.h"
#ifndef DATASET_H
#define DATASET_H

int carregarDataset(Aluno *alunos, int maxAlunos);
void normalizarAlunos(Aluno *alunos, int total);

#endif // DATASET_H