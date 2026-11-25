// Gerador de instâncias do TF de Programação Paralela (Max-Cut).
//
// Compilar: gcc -O2 -o gerador gerador.c
// Uso:      ./gerador <n> <semente> [grau_medio] > grafo.txt
//
// Gera um grafo simples (sem laços e sem arestas repetidas) com n vértices e
// m = n * grau_medio / 2 arestas sorteadas uniformemente entre todos os pares.
// Cada aresta recebe peso +1 ou -1 com igual probabilidade. O grau médio
// padrão é 10. A mesma combinação de argumentos gera sempre o mesmo arquivo,
// em qualquer máquina.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint64_t estado;

// Embaralha os bits de z (finalizador do splitmix64).
static uint64_t mistura(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// splitmix64: gerador pseudoaleatório simples e com resultado igual em
// qualquer plataforma (só usa aritmética de 64 bits sem sinal).
static uint64_t proximo(void) {
    return mistura(estado += 0x9E3779B97F4A7C15ULL);
}

static int compara(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "uso: %s <n> <semente> [grau_medio]\n", argv[0]);
        return 1;
    }
    long long n = atoll(argv[1]);
    uint64_t semente = strtoull(argv[2], NULL, 10);
    long long grau = argc == 4 ? atoll(argv[3]) : 10;
    // n cabe em int; com grau_medio <= n - 1, o grafo não passa do completo
    // e n * grau_medio não estoura 64 bits.
    if (n < 2 || n > INT32_MAX || grau < 1 || grau > n - 1) {
        fprintf(stderr, "erro: precisa de 2 <= n <= %d e 1 <= grau_medio <= n - 1\n", INT32_MAX);
        return 1;
    }
    long long m = n * grau / 2;

    // Cada aresta (u, v), com u < v, vira uma chave de 64 bits: u nos 32 bits
    // altos e v nos baixos. Nenhuma chave vale 0, porque v >= 1, então 0
    // marca posição vazia na tabela. Ordenar as chaves ordena as arestas por
    // (u, v).
    uint64_t *aresta = malloc(m * sizeof *aresta);
    size_t cap = 1;
    while (cap < 2 * (size_t)m) cap <<= 1;  // tabela no máximo meio cheia
    uint64_t *tabela = calloc(cap, sizeof *tabela);
    if (!aresta || !tabela) {
        fprintf(stderr, "erro: memória insuficiente\n");
        return 1;
    }

    estado = semente;
    long long k = 0;
    while (k < m) {
        // O resto da divisão tem viés desprezível, já que n é muito menor
        // que 2^64.
        uint64_t u = proximo() % n, v = proximo() % n;
        if (u == v) continue;
        if (u > v) { uint64_t t = u; u = v; v = t; }
        uint64_t chave = (u << 32) | v;

        // Tabela de espalhamento com sondagem linear: descarta repetidas.
        size_t i = mistura(chave) & (cap - 1);
        while (tabela[i] != 0 && tabela[i] != chave) i = (i + 1) & (cap - 1);
        if (tabela[i] == chave) continue;
        tabela[i] = chave;
        aresta[k++] = chave;
    }
    free(tabela);
    qsort(aresta, m, sizeof *aresta, compara);

    static char buffer[1 << 20];
    setvbuf(stdout, buffer, _IOFBF, sizeof buffer);
    printf("%lld %lld\n", n, m);
    for (long long i = 0; i < m; i++) {
        unsigned u = (unsigned)(aresta[i] >> 32) + 1, v = (unsigned)(aresta[i] & 0xFFFFFFFFu) + 1;
        printf("%u %u %d\n", u, v, (proximo() & 1) ? 1 : -1);
    }

    free(aresta);
    return 0;
}
