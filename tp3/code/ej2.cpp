#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <mpi.h>      // Biblioteca MPI para programación paralela distribuida
using namespace std;

// Rabin-Karp con rolling hash: devuelve la cantidad de ocurrencias de un patrón
size_t RabinKarp(const string& text, const string& pattern) {
    int n = text.size(), m = pattern.size();
    if (m == 0 || m > n) return 0;

    const unsigned long long d = 256;
    const unsigned long long q = 1000000007;

    unsigned long long h = 1, hashPattern = 0, hashText = 0;
    size_t count = 0;

    for (int i = 0; i < m - 1; i++)
        h = (h * d) % q;

    for (int i = 0; i < m; i++) {
        hashPattern = (hashPattern * d + static_cast<unsigned char>(pattern[i])) % q;
        hashText    = (hashText    * d + static_cast<unsigned char>(text[i])) % q;
    }

    for (int i = 0; i <= n - m; i++) {
        if (hashText == hashPattern){
            bool match = true;
            for (int j = 0; j < m; j++) {
                if (text[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) count++;
        }
        if (i < n - m) {
            hashText = (d * (hashText - static_cast<unsigned char>(text[i]) * h) + static_cast<unsigned char>(text[i + m])) % q;
            if (hashText < 0) hashText += q;
        }
    }
    return count;
}

vector<size_t> RabinKarpSequential(const string& text, const vector<string>& patterns) {
    vector<size_t> counts;
    counts.reserve(patterns.size());
    for (const auto& p : patterns)
        counts.push_back(RabinKarp(text, p));
    return counts;
}

bool leer_texto(const string& ruta, string& texto) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo) return false;
    texto.assign((istreambuf_iterator<char>(archivo)), istreambuf_iterator<char>());
    return true;
}

bool leer_patrones(const string& ruta, vector<string>& patterns) {
     ifstream archivo(ruta);
    if (!archivo) return false;
    string linea;
    while (getline(archivo, linea)) if (!linea.empty()) patterns.push_back(linea);
    return true;
}

int main(int argc, char** argv) {
    // Inicialización de MPI
    MPI_Init(&argc, &argv);
    
    // Obtener el número total de procesos
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Obtener el rank del proceso actual
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    string texto;
    vector<string> patterns;
    int num_patterns = 0;
    int text_size = 0;
    double tiempo_inicio, tiempo_fin;

    // Solo el proceso 0 (maestro) lee los archivos
    if (rank == 0) {
        string ruta_texto = "../texto.txt";
        string ruta_patrones = "../patrones.txt";

        if (!leer_texto(ruta_texto, texto)) {
            cerr << "No se pudo leer texto.txt\n";
            MPI_Abort(MPI_COMM_WORLD, 1); // Abortar todos los procesos MPI
        }
        if (!leer_patrones(ruta_patrones, patterns)) {
            cerr << "No se pudo leer patrones.txt\n";
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        
        num_patterns = patterns.size();
        text_size = texto.size();
        
        cout << "Texto cargado: " << text_size << " caracteres" << endl;
        cout << "Patrones cargados: " << num_patterns << endl;
    }

    // Broadcast: el proceso 0 envía el número de patrones a todos los procesos
    MPI_Bcast(&num_patterns, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Broadcast: el proceso 0 envía el tamaño del texto a todos los procesos
    MPI_Bcast(&text_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Los procesos no-maestro reservan memoria para recibir el texto
    if (rank != 0) {
        texto.resize(text_size);
    }
    
    // Broadcast: el proceso 0 envía el texto completo a todos los procesos
    // Todos necesitan el texto completo para buscar patrones
    MPI_Bcast(&texto[0], text_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    
    // Distribuir los patrones entre los procesos
    // Calcular cuántos patrones procesará cada proceso
    int patterns_per_process = num_patterns / world_size;
    int start_pattern = rank * patterns_per_process;
    int end_pattern;
    
    // El último proceso toma los patrones restantes
    if (rank == world_size - 1) {
        end_pattern = num_patterns;
    } else {
        end_pattern = start_pattern + patterns_per_process;
    }
    
    // El proceso 0 envía los patrones a cada proceso usando MPI_Send
    if (rank == 0) {
        // El proceso 0 procesa sus propios patrones
        tiempo_inicio = MPI_Wtime();
        
        // Enviar patrones a los otros procesos
        for (int dest = 1; dest < world_size; dest++) {
            int dest_start = dest * patterns_per_process;
            int dest_end = (dest == world_size - 1) ? num_patterns : dest_start + patterns_per_process;
            int dest_count = dest_end - dest_start;
            
            // Enviar el número de patrones que recibirá
            MPI_Send(&dest_count, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            
            // Enviar cada patrón
            for (int p = dest_start; p < dest_end; p++) {
                int pattern_size = patterns[p].size();
                // Enviar tamaño del patrón
                MPI_Send(&pattern_size, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
                // Enviar el patrón
                MPI_Send(&patterns[p][0], pattern_size, MPI_CHAR, dest, 2, MPI_COMM_WORLD);
            }
        }
    } else {
        // Los procesos no-maestro reciben sus patrones
        int my_pattern_count;
        MPI_Recv(&my_pattern_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        patterns.resize(my_pattern_count);
        for (int i = 0; i < my_pattern_count; i++) {
            int pattern_size;
            MPI_Recv(&pattern_size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            patterns[i].resize(pattern_size);
            MPI_Recv(&patterns[i][0], pattern_size, MPI_CHAR, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    
    // Cada proceso busca sus patrones asignados en el texto completo
    vector<size_t> local_counts;
    for (const auto& p : patterns) {
        local_counts.push_back(RabinKarp(texto, p));
    }
    
    // Reunir todos los resultados en el proceso 0
    if (rank == 0) {
        // El proceso 0 tiene sus propios resultados
        vector<size_t> all_counts = local_counts;
        
        // Recibir resultados de los otros procesos
        for (int src = 1; src < world_size; src++) {
            int src_start = src * patterns_per_process;
            int src_end = (src == world_size - 1) ? num_patterns : src_start + patterns_per_process;
            int src_count = src_end - src_start;
            
            vector<size_t> recv_counts(src_count);
            // Recibir los conteos de cada proceso
            MPI_Recv(&recv_counts[0], src_count, MPI_UNSIGNED_LONG_LONG, src, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Agregar a los resultados totales
            all_counts.insert(all_counts.end(), recv_counts.begin(), recv_counts.end());
        }
        
        tiempo_fin = MPI_Wtime();
        
        // Mostrar resultados
        cout << "\n[MPI con " << world_size << " procesos]" << endl;
        for (size_t i = 0; i < all_counts.size(); i++) {
            cout << "El patrón " << i << " aparece " << all_counts[i] << " veces\n";
        }
        
        cout << "\nTiempo MPI: " << (tiempo_fin - tiempo_inicio) * 1000.0 << " ms" << endl;
    } else {
        // Los procesos no-maestro envían sus resultados al proceso 0
        MPI_Send(&local_counts[0], local_counts.size(), MPI_UNSIGNED_LONG_LONG, 0, 3, MPI_COMM_WORLD);
    }

    // Finalización de MPI
    MPI_Finalize();
    return 0;
}
