# Projeto KMeans C

Projeto em C que aplica KMeans para agrupar alunos por média e número de faltas.

## Pré-requisitos
- GCC instalado
- `make` disponível

## Estrutura principal
- `KmeansSequencial.c` — versão sequencial
- `KMeansOpenMP.c` — versão paralelizada com OpenMP
- `Util/Kmeans.c` — utilitários da versão sequencial
- `Util/KMeansOpenMP_utils.c` — utilitários paralelizados
- `Util/CarregarDataset.c` — carregamento e normalização do dataset
- `Makefile` — compila ambas as versões e oferece alvos de execução

## Compilação usando o Makefile
Navegue até o diretório do projeto e execute:

```bash
cd /caminho/para/project-kmeans
make all
```

Isso criará os binários:
- `KmeansSequencial`
- `KmeansOpenMP`

### Compilar apenas uma versão
```bash
make KmeansSequencial
make KmeansOpenMP
```

## Executar
### Versão sequencial
```bash
./KmeansSequencial
```

### Versão OpenMP
```bash
./KmeansOpenMP <num_threads>
```

Exemplo:
```bash
./KmeansOpenMP 4
```

### Usando os alvos do Makefile
```bash
make run-seq
make run-omp
```

`make run-omp` executa `KmeansOpenMP` com o número de threads definido por `nproc`.

## Arquivo de dados
O programa espera encontrar o dataset em:

```bash
Data/Student_performance_data.csv
```

Esse caminho é relativo ao diretório do projeto onde estão os fontes.

## Saída
A execução gera um arquivo `resultados.csv` com as colunas:
- `numeroFaltas`
- `media`
- `cluster`

## Observações
- A versão OpenMP aceita o número de threads em tempo de execução.
- A versão sequencial é útil para comparar resultados e verificar comportamento sem paralelismo.
- Se você executar em outro diretório, ajuste apenas o caminho até o projeto antes de rodar os comandos.
