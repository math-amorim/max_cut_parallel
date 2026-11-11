#include <bits/stdc++.h>
using namespace std; 
#define p pair<int,int> 

int S = 1000;
double T0 = 1000;
double T1 = 0.01;
double tempo = 85;

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

int main (int argc, char* argv[]) {
    if (argc < 3) { 
        cout << "Uso é : ./programa <arquivo_entrada> <arquivo_saida>" << endl;
        return 1;
    }

    string arquivoEntrada = argv[1];
    string arquivoSaida = argv[2];

    ifstream arquivo(arquivoEntrada);
    if (!arquivo.is_open()) {
        cout << "Não foi possível abrir o arquivo de entrada." << endl;
        return 1;
    }

    cin.tie(0)->sync_with_stdio(0);

    // aqui a gente inicializa e popula a matriz de adjacencia do grafo
    int n, m; cin >> n >> m;
     
    vector<vector<p>> matriz(n + 1);

    int u, v, x;
  
    for (int j = 0; j < m; j++) {
        cin >> u >> v >> x;
        matriz[u].emplace_back(v,x);
        matriz[v].emplace_back(u,x);
    }
    arquivo.close();
    
    srand(time(nullptr));
    double t = clock(), now;


    // aqui a gente separa em grupos aleatorios 
    vector<int> grupos(n+1);
  
    for(int i = 0; i < n + 1; i++) grupos[i] = rand() % 2;
    
    // aq calcula o valor do corte inicial   
    int corte = calculoValorCorte(matriz, grupos, n);  
        
    // aq calcula o delta de cada vertice 
    vector<int> delta(n + 1);

    for(int i = 1; i <= n; i++) delta[i] = calcularDelta(matriz, grupos, i);
    
    double T;

    // annealing 
    for(int s = 0; ; s++) {
        now = (clock() - t) / CLOCKS_PER_SEC;
        if(now >= tempo) break;

        T = T0 * pow(T1 / T0, (double)s / (S-1));
        
        for(int i = 1; i <= n; i++) {
            if(delta[i] >= 0 or (double)rand() / RAND_MAX < exp(delta[i] / T)) {
                for(auto v : matriz[i]) {
                    int j = v.first;
                    int peso = v.second;

                    if(grupos[i] != grupos[j]) delta[j] += 2 * peso;
                    else delta[j] -= 2 * peso;   
                }
                trocaGrupo(i, grupos, delta, corte);
            }

            now = (clock() - t) / CLOCKS_PER_SEC;
            if (now >= tempo) break;
        }
    }

    string resultado;
    for(int i = 1; i <= n; i++) resultado += (grupos[i] + '0');

    gravarResultado(arquivoSaida, resultado);

    return 0;
 }
