# Max-Cut Paralelo

Trabalho foi a implementação paralela de uma solução aproximada para o problema Max-Cut em grafos ponderados não direcionados, utilizando **Simulated Annealing**, **MPI** e **OpenMP**.

## 1. Sobre o problema

O problema Max-Cut consiste em dividir os vértices de um grafo em dois conjuntos disjuntos, de modo a maximizar a soma dos pesos das arestas que conectam vértices pertencentes a conjuntos diferentes.

Como o problema é NP-difícil, utiliza-se uma abordagem heurística baseada em Simulated Annealing para encontrar boas soluções dentro de um limite de tempo.

## 2. Estratégia de solução

A implementação utiliza Simulated Annealing combinado com paralelismo híbrido (MPI + OpenMP).

### Simulated Annealing

Cada execução mantém uma solução candidata representada pela atribuição de cada vértice a um dos dois conjuntos e a cada iteração são avaliadas possíveis mudanças na partição, movendo vértices entre os conjuntos.

A variação do valor do corte é calculada incrementalmente o que evita recalcular o valor total da solução a cada movimento.

Movimentos que melhoram a solução são aceitos e os movimentos que pioram a solução podem ser aceitos de acordo com a fórmula da probabilidade. 

Essa estratégia permite escapar de ótimos locais e explorar diferentes regiões do espaço de soluções.

### Paralelismo com OpenMP

Dentro de cada processo MPI, utiliza-se OpenMP para paralelizar a avaliação e execução dos movimentos, usando coloração de grafos para permitir atualizações paralelas sem conflitos entre vértices. O grafo é colorido utilizando uma estratégia gulosa.

Os vértices de uma mesma classe de cor não possuem arestas entre si então é possível processar paralelamente os seus movimentos. Também são utilizadas operações atômicas para sincronizar as atualizações compartilhadas.

### Paralelismo com MPI

A implementação utiliza múltiplos processos MPI, cada um executando uma busca independente. Cada processo vai ter a sua própria solução, gerador de números aleatórios e temperatura.

Os processos vizinhos de tempo em tempo trocam informações sobre suas soluções e podem trocar suas temperaturas utilizando um mecanismo de troca inspirado em **Parallel Tempering** permitindo que diferentes processos explorem o espaço de soluções em diferentes regimes de temperatura.

### Controle de temperatura e tempo

As temperaturas iniciais são distribuídas entre os processos e um fator de resfriamento global é aplicado progressivamente durante a execução, alterando o comportamento da busca ao longo do tempo.

A ideia do trabalho era uma competição entre os alunos buscando a melhor solução entre execuções de cada aluno em um tempo de 90 segundos, logo existe uma constante que verifica justamente o limite de tempo e a execução do programa é interrompida quando esse limite de tempo estabelecido é atingido.

No fim os processos MPI identificam a melhor solução encontrada globalmente e o processo de rank 0 grava o resultado no arquivo de saída.

## 3. Compilação

Para compilar basta usar o Makefile:

```bash
make
```

Para remover os arquivos gerados pela compilação:

```bash
make clean
```

## 4. Execução

O programa recebe dois argumentos:

1. Caminho do arquivo de entrada.
2. Caminho do arquivo de saída.

A execução pode ser realizada utilizando:

```bash
OMP_NUM_THREADS=4 mpirun -np 4 ./main entrada.txt saida.txt
```

Onde:
- `OMP_NUM_THREADS=4`: define a quantidade de threads OpenMP por processo.
- `-np 4`: define a quantidade de processos MPI.
- `entrada.txt`: arquivo contendo a descrição do grafo.
- `saida.txt`: arquivo onde será armazenada a solução encontrada.

A quantidade de processos e threads pode ser ajustada conforme os recursos disponíveis.

## 6. Entrada

Foi utilizado o gerador de grafos disponibilidados em aula. É possível ver mais sobre no README disponibilizado em sala na pasta *gerador*.

## 7. Formato de saída

O arquivo de saída contém uma sequência de `n` caracteres binários (`0` ou `1`), representando a partição encontrada.

Cada posição corresponde a um vértice do grafo: 0 indica que o vértice pertence ao primeiro conjunto e 1 que pertence ao segundo conjunto.

O valor do corte encontrado também é exibido no terminal ao final da execução.

## 8. Parâmetros de execução

Os principais parâmetros do algoritmo podem ser ajustados diretamente no código-fonte:

| Parâmetro | Valor inicial | Descrição |
|---|---:|---|
| `Tmin` | 0.30 | Temperatura mínima |
| `Tmax` | 1.00 | Temperatura máxima |
| `G0` | 4.0 | Fator inicial de resfriamento |
| `G1` | 1.0 | Fator final de resfriamento |
| `LIMITE_TEMPO` | 83.5 s | Limite de tempo da busca |

A semente utilizada para geração de números aleatórios é inicializada pelo processo de rank 0 e compartilhada com os demais processos, garantindo que todos utilizem a mesma semente-base, com geradores independentes para cada finalidade e processo.

