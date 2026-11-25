# TF de Programação Paralela: gerador e entradas de exemplo

- `gerador.c` gera instâncias no formato do enunciado.
- `verificador.c` calcula o valor do corte de uma solução.
- `entradas/` traz instâncias prontas para o desenvolvimento.

## Compilar

```
make
```

`make entradas` recria as instâncias geradas de `entradas/` e `make clean` apaga os executáveis.

## Gerar uma instância

```
./gerador <n> <semente> [grau_medio] > grafo.txt
```

O grau médio padrão é 10, o mesmo das instâncias da competição, e o grafo tem m = n × grau_medio / 2 arestas. Os mesmos argumentos geram o mesmo arquivo em qualquer máquina. As instâncias Média e Grande da competição vêm deste gerador, com n = 20.000 e n = 100.000 e sementes que só serão conhecidas na avaliação.

As arestas saem ordenadas por (u, v), com u < v, sem laços e sem repetição. Alguns vértices podem ficar isolados (grau 0): a instância `n100000_s1.txt` tem 6. O programa precisa aceitar esses vértices, e o lado em que ficam não altera o corte.

## Verificar uma solução

```
./verificador grafo.txt solucao.txt
```

Imprime o valor do corte. Se a solução não tiver exatamente n caracteres `0` ou `1` na primeira linha, imprime o motivo e termina com código 1.

## Entradas de exemplo

| Arquivo | n | m | Melhor corte conhecido | Origem do valor |
| --- | ---: | ---: | ---: | --- |
| `exemplo_n5.txt` | 5 | 6 | 3 | ótimo, por força bruta (exemplo do enunciado) |
| `n20_s1.txt` | 20 | 100 | 18 | ótimo, por força bruta |
| `n100_s1.txt` | 100 | 500 | 118 | *Simulated Annealing* de referência |
| `n1000_s1.txt` | 1.000 | 5.000 | 1.178 | *Simulated Annealing* de referência (instância Treino) |
| `n20000_s1.txt` | 20.000 | 100.000 | 23.668 | *Simulated Annealing* de referência |
| `n100000_s1.txt` | 100.000 | 500.000 | 116.548 | *Simulated Annealing* de referência |

O *Simulated Annealing* de referência é a versão sequencial do Algoritmo 1 do enunciado, com 10.000 varreduras, T0 = 1,5 e T1 = 0,3, e o valor é o melhor de 3 sementes. Esses valores não são ótimos provados e podem ser superados. Na instância de 20 vértices, ele também chegou ao ótimo, 18.

Todas as instâncias `n*_s1.txt` foram geradas com semente 1 e podem ser recriadas localmente, por exemplo com `./gerador 100000 1 > n100000_s1.txt`.
