#include "../Model/aluno.h"
#ifndef DATASET_H
#define DATASET_H

char *retornarDadoPorIndice(char *linha, int index);
int carregarDataset(Aluno *alunos);
void normalizarAlunos(Aluno *alunos, int total);
int obterNumeroLinhas(const char *filename);

#endif // DATASET_H