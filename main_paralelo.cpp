#include <bits/stdc++.h>
#include <mpi.h>
#include <chrono>
using namespace std; 
using namespace chrono;
#define p pair<int,int> 

int S = 10000;
double tempo = 1;

int calculoValorCorte (vector<vector<p>>&matriz, vector<int>&grupos, int n) { 
    int corte = 0;

    for(int u = 1; u <= n; u++) {
        for(auto v : matriz[u]) 
            if(u < v.first && grupos[u] != grupos[v.first])  corte += v.second;
    }

    return corte;
} 

int calcularDelta (vector<vector<p>>&matriz, vector<int>&grupos, int i) { 
    int delta = 0;

    for(auto v : matriz[i]) { 
        if(grupos[i] != grupos[v.first]) delta -= v.second;
        else delta += v.second;
    }

    return delta;
}

void trocaGrupo(int i, vector<int>& grupos, vector<int>& delta, int& corte) {
    // adiciono o ganho ou a perda ao meu corte
    corte += delta[i];

    // inverto o grupo 
    grupos[i] ^= 1; 

    // e ai indico que agora realizar a inversão novamente vai acrescentar ou subtrair o valor de delta novamente
    delta[i] = -delta[i];

}

void gravarResultado(string arquivoSaida, string resultado) { 
    ofstream arquivo(arquivoSaida); 

    if(arquivo.is_open()) { 
        arquivo << resultado << endl;
    } else cout << "Não foi possível abrir o arquivo." << endl;
}

void comunicacaoMPI (int rank, int processos, int fase, vector<vector<p>>& matriz, int n, vector<int>& grupos, vector<int>& delta, int& corte, int& melhorCorte, vector<int>& melhorGrupos, double T, vector<double>& temperaturas) { 
    int vizinho = -1; 

    // vejo quem vou ver nessa fase 
    if (fase == 0) vizinho = (rank % 2 == 0) ? rank + 1 : rank - 1; 
    else vizinho = (rank % 2 == 0) ? rank - 1 : rank + 1;

    // vejo se vizinha é valido 
    if (vizinho < 0 or vizinho >= processos) return;
    
    // troco valor corte
    int corteVizinho = 0;

    MPI_Sendrecv(&corte, 1, MPI_INT, vizinho, 0, &corteVizinho, 1, MPI_INT, vizinho, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // processo c rank menor faz a decisão 
    int decisao = 0;

    if (rank < vizinho) { 
        double expoente = (1.0 / T - 1.0 / temperaturas[vizinho]) * (corteVizinho - corte);
        
        double probabilidade = min(1.0, exp(expoente));

        double sorteio = (double)rand() / (RAND_MAX + 1.0);

        decisao = sorteio < probabilidade;
    }

    int decisaoRecebida = 0; 

    MPI_Sendrecv(&decisao, 1, MPI_INT, vizinho, 1, &decisaoRecebida, 1, MPI_INT, vizinho, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // processo de maior rank receb decisao 

    if (rank > vizinho) decisao = decisaoRecebida;


    if (decisao) { 
        vector<int> gruposVizinho(n + 1);

        MPI_Sendrecv(grupos.data(), n + 1, MPI_INT, vizinho, 2, gruposVizinho.data(), n + 1, MPI_INT, vizinho, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // atualizo meu corte e grupos
        corte = corteVizinho;
        grupos.swap(gruposVizinho);

        for (int i = 1; i <= n; i++) delta[i] = calcularDelta(matriz, grupos, i);

        if (corte > melhorCorte) melhorCorte = corte, melhorGrupos = grupos;

    }
  /*  if (rank < vizinho) {
        cout << "Troca entre ranks " << rank << " e " << vizinho
             << " | cortes: " << corte << " / " << corteVizinho
             << " | aceita: " << decisao << endl;
    }
*/
}

void trocaTemperatura(int processos, int fase, int& idx, double& T, int corte,
                      const vector<double>& escada,
                      vector<long long>& tent, vector<long long>& ace,
                      mt19937& rng) {
    // todos participam (coletiva): descobre quem tem cada temperatura
    vector<int> idxDe(processos), quem(processos);
    MPI_Allgather(&idx, 1, MPI_INT, idxDe.data(), 1, MPI_INT, MPI_COMM_WORLD);
    for (int r = 0; r < processos; r++) quem[idxDe[r]] = r;

    // pares de índices: fase 0 -> (0,1)(2,3) ; fase 1 -> (1,2)
    int idxViz = (idx % 2 == fase) ? idx + 1 : idx - 1;
    if (idxViz < 0 || idxViz >= processos) return;
    int vizinho = quem[idxViz];

    int corteViz;
    MPI_Sendrecv(&corte, 1, MPI_INT, vizinho, 0,
                 &corteViz, 1, MPI_INT, vizinho, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // quem tem a temperatura mais fria decide
    int decisao = 0;
    if (idx < idxViz) {
        double expoente = (1.0 / escada[idx] - 1.0 / escada[idxViz]) * (corteViz - corte);
        double prob = expoente >= 0 ? 1.0 : exp(expoente);
        decisao = uniform_real_distribution<double>(0.0, 1.0)(rng) < prob;
        tent[idx]++;
        if (decisao) ace[idx]++;
    }

    int recebida = 0;
    MPI_Sendrecv(&decisao, 1, MPI_INT, vizinho, 1,
                 &recebida, 1, MPI_INT, vizinho, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (idx > idxViz) decisao = recebida;

    if (decisao) {       // só troca a temperatura, grupos e delta continuam válidos
        idx = idxViz;
        T = escada[idx];
    }
}

int main (int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, processos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    vector<double> temperaturas = {0.35, 0.45, 0.57, 0.7};    
    if (processos != 4) { 
        if (rank == 0) cout << "O programa precisa ser executado com 4 processos." << endl;

        MPI_Finalize();
        return 1;
    }

    double T = temperaturas[rank];

    auto inicio = steady_clock::now();
    //double t = clock(), now;
    
    if (argc < 3) { 
        cout << "Uso é : ./programa <arquivo_entrada> <arquivo_saida>" << endl;
        
        MPI_Finalize();
        return 1;
    }

    string arquivoEntrada = argv[1];
    string arquivoSaida = argv[2];

    ifstream arquivo(arquivoEntrada);
    if (!arquivo.is_open()) {
        cout << "Não foi possível abrir o arquivo de entrada." << endl;
       
        MPI_Finalize();
        return 1;
    }

    cin.tie(0)->sync_with_stdio(0);

    // aqui a gente inicializa e popula a matriz de adjacencia do grafo
    int n, m; arquivo >> n >> m;
    vector<vector<p>> matriz(n + 1);

    int u, v, x;
  
    for (int j = 0; j < m; j++) {
        arquivo >> u >> v >> x;
        matriz[u].emplace_back(v,x);
        matriz[v].emplace_back(u,x);
    }
    arquivo.close();
    srand(time(nullptr) + rank);

    // aqui a gente separa em grupos aleatorios 
    vector<int> grupos(n+1);
  
    for(int i = 0; i < n + 1; i++) grupos[i] = rand() % 2;
    
    // aq calcula o valor do corte inicial   
    int corte = calculoValorCorte(matriz, grupos, n);  
        
    int melhorCorte = corte;
    vector<int> melhorGrupos = grupos; 

    // aq calcula o delta de cada vertice 
    vector<int> delta(n + 1);

    for(int i = 1; i <= n; i++) delta[i] = calcularDelta(matriz, grupos, i);
    
    int fase = 0;
    // annealing 
    for(int s = 0; s < 100000; s++) {
        auto agora = steady_clock::now();
        double now = duration_cast<duration<double>>(agora - inicio).count();
        //now = (clock() - t) / CLOCKS_PER_SEC;

//        if(now >= tempo) break;
        
        for(int i = 1; i <= n; i++) {
            if (i % 1000 == 0) { 
                agora = steady_clock::now();
                now = duration_cast<duration<double>>(agora - inicio).count();
  //              if (now >= tempo) break;
            }
            
            if(delta[i] >= 0 or (double)rand() / RAND_MAX < exp(delta[i] / T)) {
                for(auto v : matriz[i]) {
                    int j = v.first;
                    int peso = v.second;

                    if(grupos[i] != grupos[j]) delta[j] += 2 * peso;
                    else delta[j] -= 2 * peso;   
                }
                trocaGrupo(i, grupos, delta, corte);
            
                if(corte > melhorCorte) melhorCorte = corte, melhorGrupos = grupos;     
            }

            //agora = steady_clock::now();
            // now = duration_cast<duration<double>>(agora - inicio).count();
            // now = (clock() - t) / CLOCKS_PER_SEC;
            // if (now >= tempo) break;
        }
        if ((s + 1) % 10 == 0) {
        comunicacaoMPI(
            rank,
            processos,
            fase,
            matriz,
            n,
            grupos,
            delta,
            corte,
            melhorCorte,
            melhorGrupos,
            T,
            temperaturas
        );

    fase = 1 - fase;
}
    }

cout << "Rank " << rank
     << " | Melhor corte local: " << melhorCorte
     << endl;

    int melhorGlobal, melhorRank;

    MPI_Allreduce(&melhorCorte, &melhorGlobal, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    int candidato = (melhorCorte == melhorGlobal) ? rank : processos;

    MPI_Reduce(&candidato, &melhorRank, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Bcast(&melhorRank, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "Melhor corte global: " << melhorGlobal << endl;
        cout << "Rank vencedor: " << melhorRank << endl;
    }

    if (rank == melhorRank and rank != 0) 
        MPI_Send(melhorGrupos.data(), n + 1, MPI_INT, 0, 10, MPI_COMM_WORLD);

    if (rank == 0 and melhorRank != 0) 
        MPI_Recv(melhorGrupos.data(), n + 1, MPI_INT, melhorRank, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (rank == 0) { 
        string resultado;
        for(int i = 1; i <= n; i++) resultado += (melhorGrupos[i] + '0');

        gravarResultado(arquivoSaida, resultado);

       // cout << "Divisão dos grupos foi: " << resultado << endl;
        //cout << "Valor do corte foi: " << melhorCorte << endl;
    }
    MPI_Finalize();

    return 0;
 }
