// Verificador de soluções do TF de Programação Paralela (Max-Cut).
//
// Compilar: gcc -O2 -o verificador verificador.c
// Uso:      ./verificador grafo.txt solucao.txt
//
// Imprime o valor do corte da solução. Se a solução for inválida (tamanho
// diferente de n ou caracteres que não sejam 0 e 1), imprime o motivo e
// termina com código de saída 1.

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s grafo.txt solucao.txt\n", argv[0]);
        return 1;
    }
    FILE *g = fopen(argv[1], "r"), *s = fopen(argv[2], "r");
    if (!g || !s) {
        fprintf(stderr, "erro: não foi possível abrir %s\n", !g ? argv[1] : argv[2]);
        return 1;
    }

    long long n, m;
    if (fscanf(g, "%lld %lld", &n, &m) != 2 || n < 1 || m < 0) {
        fprintf(stderr, "erro: cabeçalho do grafo inválido\n");
        return 1;
    }

    // A solução é uma linha com exatamente n caracteres 0 ou 1.
    char *x = malloc(n);
    if (!x) {
        fprintf(stderr, "erro: memória insuficiente\n");
        return 1;
    }
    long long k = 0;
    int c;
    while ((c = fgetc(s)) != EOF && c != '\n' && c != '\r') {
        if (c != '0' && c != '1') {
            printf("inválida: caractere '%c' na posição %lld\n", c, k + 1);
            return 1;
        }
        if (k == n) {
            printf("inválida: mais de %lld caracteres\n", n);
            return 1;
        }
        x[k++] = (char)c;
    }
    if (k != n) {
        printf("inválida: %lld caracteres, esperado %lld\n", k, n);
        return 1;
    }

    long long corte = 0, u, v, w;
    for (long long i = 0; i < m; i++) {
        if (fscanf(g, "%lld %lld %lld", &u, &v, &w) != 3 || u < 1 || u > n || v < 1 || v > n) {
            fprintf(stderr, "erro: aresta %lld do grafo inválida\n", i + 1);
            return 1;
        }
        if (x[u - 1] != x[v - 1]) corte += w;
    }

    printf("%lld\n", corte);
    return 0;
}
