#include <bits/stdc++.h>
#include <mpi.h>
#include <chrono>
#include <omp.h>
using namespace std; 
using namespace chrono;
#define p pair<int,int> 

double LIMITE_TEMPO = 85.0;

int calculoValorCorte (vector<vector<p>>&matriz, vector<int>&grupos, int n) { 
    int corte = 0;

    for(int u = 1; u <= n; u++) {
        for(auto v : matriz[u]) 
            if(u < v.first && grupos[u] != grupos[v.first]) corte += v.second;
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
    #pragma omp atomic update
    corte += delta[i];

    // inverto o grupo 
    grupos[i] ^= 1; 

    // e ai indico que agora realizar a inversão novamente vai acrescentar ou subtrair o valor de delta novamente
    delta[i] = -delta[i];

}

void gravarResultado(string arquivoSaida, string resultado) { 
    ofstream arquivo(arquivoSaida); 

    if(arquivo.is_open()) arquivo << resultado << endl;
    else cout << "Não foi possível abrir o arquivo." << endl;
}

void trocaTemperatura (int processos, int fase, int& idxTemperatura, double& T, int corte, const vector<double>&temperaturas, long long& tentativas, long long& aceitas, mt19937& rng) { 

    vector<int> indices(processos);

    MPI_Allgather(&idxTemperatura, 1, MPI_INT, indices.data(), 1, MPI_INT, MPI_COMM_WORLD);

    int idxVizinho; 

    if (idxTemperatura % 2 == fase) idxVizinho = idxTemperatura + 1;
    else idxVizinho = idxTemperatura - 1;

    if (idxVizinho < 0 or idxVizinho >= processos) return;

    int vizinho = -1;

    for (int r = 0; r < processos; r++) {
        if (indices[r] == idxVizinho) {
            vizinho = r;
            break;
        }
    }

    int idxVizinhoRecebido;

    MPI_Sendrecv(&idxTemperatura, 1, MPI_INT, vizinho, 0, &idxVizinhoRecebido, 1, MPI_INT, vizinho, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int corteVizinho;

    MPI_Sendrecv(&corte, 1, MPI_INT, vizinho, 1, &corteVizinho, 1, MPI_INT, vizinho, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int decisao = 0;

    if (idxTemperatura < idxVizinhoRecebido) { 
        double TA = temperaturas[idxTemperatura];
        double TB = temperaturas[idxVizinho];

        double expoente = (1.0 / TA - 1.0 / TB) * (corteVizinho - corte);

        double probabilidade = (expoente >= 0.0) ? 1.0 : exp(expoente);

        uniform_real_distribution<double> dist(0.0, 1.0);

        decisao = (dist(rng) < probabilidade);

        tentativas++;
        if (decisao) aceitas++;
    }

    int decisaoRecebida = 0;

    MPI_Sendrecv(&decisao, 1, MPI_INT, vizinho, 2, &decisaoRecebida, 1, MPI_INT, vizinho, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (idxTemperatura > idxVizinhoRecebido) decisao = decisaoRecebida;

    if (decisao) idxTemperatura = idxVizinho, T = temperaturas[idxTemperatura];
} 

vector<vector<int>> colorirGrafo(vector<vector<p>>&matriz, int n) { 
    vector<int> cor(n+1, -1);
    vector<int> marcadas(n+1, -1); 
    vector<vector<int>> cores;

    for (int i = 1; i <= n; i++) { 
        for (auto v : matriz[i]) { 
            int vizinho = v.first; 
            if(cor[vizinho] != -1) marcadas[cor[vizinho]] = i;
        }

        int c = 0; 

        while (marcadas[c] == i) c++;

        cor[i] = c; 

        if (c >= (int) cores.size()) cores.resize(c + 1);

        cores[c].push_back(i);
    }

    return cores;
}

mt19937 criarGerador(uint64_t seedBase, int rank, int finalidade, int thread = 0) {
    seed_seq sequencia {
        (uint32_t)seedBase,
        (uint32_t)(seedBase >> 32),
        (uint32_t)rank,
        (uint32_t)finalidade,
        (uint32_t)thread
    };

    return mt19937(sequencia);
}

int main (int argc, char* argv[]) {
    int fornecido;

    MPI_Init_thread(
        &argc,
        &argv,
        MPI_THREAD_FUNNELED,
        &fornecido
    );

    if (fornecido < MPI_THREAD_FUNNELED) MPI_Abort(MPI_COMM_WORLD, 1);
    
    int rank, processos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    uint64_t seed = 0; 

    random_device rd; 
    seed = ((uint64_t) rd() << 32) ^ rd(); 
    seed ^= (uint64_t)steady_clock::now().time_since_epoch().count();


    MPI_Bcast(&seed, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

    vector<double> temperaturas = {0.40, 0.70, 1.00, 1.61};
    
    int idxTemperatura = rank; 
    double T = temperaturas[idxTemperatura];

    long long tentativas = 0, aceitas = 0;
    
    mt19937 rng = criarGerador(seed, rank, 0);

    MPI_Barrier(MPI_COMM_WORLD);
    auto inicio = steady_clock::now();
    
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

    vector<vector<int>> cores = colorirGrafo(matriz, n);

    mt19937 rngGrupos = criarGerador(seed, rank, 1); 

    uniform_int_distribution<int> distGrupo(0,1);

    // aqui a gente separa em grupos aleatorios 
    vector<int> grupos(n+1);
    for(int i = 0; i < n + 1; i++) grupos[i] = distGrupo(rngGrupos);
    
    // aq calcula o valor do corte inicial   
    int corte = calculoValorCorte(matriz, grupos, n);  

    int melhorCorte = corte;
    vector<int> melhorGrupos = grupos; 

    // aq calcula o delta de cada vertice 
    vector<int> delta(n + 1);
    for(int i = 1; i <= n; i++) delta[i] = calcularDelta(matriz, grupos, i);

    // inicializa thread
    int numThreads = omp_get_max_threads();

    vector<vector<int>> negativosPorThread(numThreads); 
    vector<mt19937> geradores(numThreads);

    for (int i = 0; i < numThreads; i++) geradores[i] = criarGerador(seed, rank, 2, i);

    int fase = 0, s = 0, continuar = 1;
    
    // annealing 
    #pragma omp parallel
    { 
        while (continuar) {        
            int tid = omp_get_thread_num(); 
            uniform_real_distribution<double> dist(0.0, 1.0);

            for (int c = 0; c < (int)cores.size(); c++) { 
                #pragma omp for schedule(static)
                for (int k = 0; k < (int)cores[c].size(); k++) { 
                    int i = cores[c][k]; 

                    if(delta[i] >= 0 or dist(geradores[tid]) < exp(delta[i] / T)) {
                        if (delta[i] < 0) negativosPorThread[tid].push_back(i);
                        
                        for(auto v : matriz[i]) {
                            int j = v.first;
                            int peso = v.second;

                            int valor;
                            if(grupos[i] != grupos[j]) valor = 2 * peso;
                            else valor = -2 * peso;
                            
                            #pragma omp atomic update
                            delta[j] += valor;
                        }
                        trocaGrupo(i, grupos, delta, corte);
                       
                    }
                }

                #pragma omp single
                { 
                    int corteCandidato = corte;

                    for (int t = 0; t < numThreads; t++) { 
                        for (int i : negativosPorThread[t]) 
                            corteCandidato += delta[i]; 
                    }

                    if (corteCandidato > melhorCorte) {
                        melhorCorte = corteCandidato;
                        melhorGrupos = grupos;

                        for (int t = 0; t < numThreads; t++) {
                            for (int i : negativosPorThread[t])
                                melhorGrupos[i] ^= 1;
                        }
                    }

                    for (int t = 0; t < numThreads; t++) negativosPorThread[t].clear();
                }
        }

            #pragma omp master
            {
                s++; 
                
                if (s % 10 == 0) {
                    trocaTemperatura(processos, fase, idxTemperatura, T, corte, temperaturas, tentativas, aceitas, rng);
                    fase = 1 - fase;
                

                    if (rank == 0) { 
                        auto agora = steady_clock::now();
                        double now = duration_cast<duration<double>>(agora - inicio).count();
                        if (now >= LIMITE_TEMPO) continuar = 0;
                    }

                    MPI_Bcast(&continuar, 1, MPI_INT, 0, MPI_COMM_WORLD);
                }
            }
                #pragma omp barrier
        }
    }

    int melhorGlobal, melhorRank;

    // seleciona melhor corte e distribui para cada oricessi
    MPI_Allreduce(&melhorCorte, &melhorGlobal, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    // identifica o rank vendo se eu achei o corte ou não
    int candidato = (melhorCorte == melhorGlobal) ? rank : processos;

    // acho o candidato com menor rank que tem o melhor corte
    MPI_Reduce(&candidato, &melhorRank, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Bcast(&melhorRank, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0 and rank == melhorRank) MPI_Send(melhorGrupos.data(), n + 1, MPI_INT, 0, 10, MPI_COMM_WORLD);

    if (rank == 0 and melhorRank != 0) MPI_Recv(melhorGrupos.data(), n + 1, MPI_INT, melhorRank, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    if (rank == 0) { 
        string resultado;
        for(int i = 1; i <= n; i++) resultado += (melhorGrupos[i] + '0');

        gravarResultado(arquivoSaida, resultado);

        cout << "Valor do corte foi: " << melhorGlobal << endl;
    }

    MPI_Finalize();

    return 0;
 }
