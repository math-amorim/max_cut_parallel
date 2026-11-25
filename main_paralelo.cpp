#include <bits/stdc++.h>
#include <mpi.h>
#include <chrono>
#include <omp.h>
using namespace std; 
using namespace chrono;
#define p pair<int,int> 

double Tmin = 0.30;
double Tmax = 1.00;
double G0   = 4.0;
double G1   = 1.0;
double LIMITE_TEMPO = 83.5;

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
    #pragma omp atomic update
    corte += delta[i];

    grupos[i] ^= 1; 

    delta[i] = -delta[i];

}

void gravarResultado(string arquivoSaida, string resultado) { 
    ofstream arquivo(arquivoSaida); 

    if(arquivo.is_open()) arquivo << resultado << endl;
    else cout << "Não foi possível abrir o arquivo." << endl;
}

void trocaTemperatura (int processos, int fase, int& idxTemperatura, double& T, int corte, const vector<double>&temperaturas, mt19937& rng, double g) { 
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
        double TA = temperaturas[idxTemperatura] * g;
        double TB = temperaturas[idxVizinho] * g;

        double expoente = (1.0 / TA - 1.0 / TB) * (corteVizinho - corte);

        double probabilidade = (expoente >= 0.0) ? 1.0 : exp(expoente);

        uniform_real_distribution<double> dist(0.0, 1.0);

        decisao = (dist(rng) < probabilidade);
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

    if (rank == 0) {
        random_device rd;
        seed = ((uint64_t)rd() << 32) ^ rd();
        seed ^= (uint64_t)steady_clock::now().time_since_epoch().count();
    }

    MPI_Bcast(&seed, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

    vector<double> temperaturas(processos);
    
    for (int i  = 0; i < processos; i++) { 
        double proporcao = (processos > 1) ? (double)i / (processos - 1) : 0.0;        
        
        temperaturas[i] = Tmin * pow(Tmax / Tmin, proporcao);
    }

    int idxTemperatura = rank; 
    double T = temperaturas[idxTemperatura];
    
    double g = G0;

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

    FILE* arquivo = fopen(arquivoEntrada.c_str(), "r");
    
    if (arquivo == nullptr) {
        cout << "Não foi possível abrir o arquivo de entrada." << endl;
       
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int n, m; 
    if (fscanf(arquivo, "%d %d", &n, &m) != 2) {
        cerr << "Erro ao ler n e m." << endl;
        fclose(arquivo);
        MPI_Abort(MPI_COMM_WORLD, 1);
    
    }

    vector<vector<p>> matriz(n + 1);

    int u, v, x;
  
    for (int j = 0; j < m; j++) {
        if (fscanf(arquivo, "%d %d %d", &u, &v, &x) != 3) {
            cerr << "Erro ao ler aresta." << endl;
            fclose(arquivo);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        matriz[u].emplace_back(v,x);
        matriz[v].emplace_back(u,x);
    }
    fclose(arquivo);

    int grauMax = 0;
    for (int i = 1; i <= n; i++) grauMax = max(grauMax, (int)matriz[i].size());
    
    vector<double> tabelaExp(grauMax + 1);

    vector<vector<int>> cores = colorirGrafo(matriz, n);

    mt19937 rngGrupos = criarGerador(seed, rank, 1); 

    uniform_int_distribution<int> distGrupo(0,1);

    vector<int> grupos(n+1);
    for(int i = 0; i < n + 1; i++) grupos[i] = distGrupo(rngGrupos);
    
    int corte = calculoValorCorte(matriz, grupos, n);  

    int melhorCorte = corte;
    vector<int> melhorGrupos = grupos; 

    vector<int> delta(n + 1);
    for(int i = 1; i <= n; i++) delta[i] = calcularDelta(matriz, grupos, i);

    int numThreads = omp_get_max_threads();

    vector<mt19937> geradores(numThreads);

    for (int i = 0; i < numThreads; i++) geradores[i] = criarGerador(seed, rank, 2, i);

    int fase = 0, s = 0, continuar = 1;
    double temperaturaTabela = -1.0;

    #pragma omp parallel
    { 
        while (continuar) {   
            #pragma omp single
            {
                double Tuso = T * g;
                if (temperaturaTabela != Tuso) 
                    for (int k = 0; k <= grauMax; k++) tabelaExp[k] = exp(-k / Tuso);
            
                temperaturaTabela = Tuso;
            }    
            int tid = omp_get_thread_num(); 
            uniform_real_distribution<double> dist(0.0, 1.0);

            for (int c = 0; c < (int)cores.size(); c++) { 
                #pragma omp for schedule(static)
                for (int k = 0; k < (int)cores[c].size(); k++) { 
                    int i = cores[c][k]; 

                    if(delta[i] >= 0 or dist(geradores[tid]) < tabelaExp[-delta[i]]) {                        
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
            }

            #pragma omp single
            {
                if (corte > melhorCorte) melhorCorte = corte, melhorGrupos = grupos;
            }

            #pragma omp master
            {
                s++; 
                
                if (s % 10 == 0) {
                    trocaTemperatura(processos, fase, idxTemperatura, T, corte, temperaturas, rng, g);
                    
                    fase = 1 - fase;
                    
                    double flags[2] = {1.0, g};

                    if (rank == 0) { 
                        double now = duration_cast<duration<double>>(steady_clock::now() - inicio).count();
                        
                        if (now >= LIMITE_TEMPO) flags[0] = 0;

                        double frac = min(1.0, now / LIMITE_TEMPO); 
                        
                        flags[1] = G0 * pow (G1 / G0, frac);
                    }

    
                    MPI_Bcast(flags, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

                    continuar = flags[0];
                    g = flags[1];
                }
            }
                #pragma omp barrier
        }
    }

    int melhorGlobal, melhorRank;

    MPI_Allreduce(&melhorCorte, &melhorGlobal, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    int candidato = (melhorCorte == melhorGlobal) ? rank : processos;

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
