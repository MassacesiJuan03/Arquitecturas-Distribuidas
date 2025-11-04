#include <bits/stdc++.h>
#include <mpi.h>      // Biblioteca MPI para programación paralela distribuida
#include <chrono>
using namespace std;

// --------------------
// Chequear primalidad usando primos_base
// Verifica si un número n es primo usando los primos base como divisores
// --------------------
bool esPrimo(long long n, const vector<long long>& primos_base) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;  // Descartar pares
    
    // Solo necesitamos verificar divisibilidad hasta sqrt(n)
    for (auto p : primos_base) {
        if (p * p > n) break;      // condición de corte
        if (n % p == 0) return false;
    }
    return true;
}

// --------------------
// Paso 1: generar primos base hasta sqrt(N)
// Estos primos se usan para verificar la primalidad de números mayores
// --------------------
vector<long long> generarPrimosBase(long long limite) {
    vector<long long> primos;
    if (limite >= 2) primos.push_back(2);
    
    for (long long i = 3; i <= limite; i += 2) {  // Solo números impares
        bool primo = true;
        for (long long j = 3; j * j <= i; j += 2) {
            if (i % j == 0) { primo = false; break; }
        }
        if (primo) primos.push_back(i);
    }
    return primos;
}

// --------------------
// SECUENCIAL (pasos 1-3)
// --------------------
vector<long long> primosSecuencial(long long N) {
    vector<long long> primos_base = generarPrimosBase(sqrt(N));
    vector<long long> primos;

    for (long long i = 2; i <= N; i++) {
        if (esPrimo(i, primos_base)) primos.push_back(i);
    }
    return primos;
}

// --------------------
// MPI: Cada proceso busca primos en su rango asignado
// --------------------
vector<long long> primosMPI(long long N, int rank, int world_size, 
                             const vector<long long>& primos_base) {
    vector<long long> primos_locales;
    
    // Calcular el rango de números que este proceso verificará
    long long bloque = N / world_size;
    long long inicio = rank * bloque;
    long long fin;
    
    if (rank == 0) {
        inicio = 2;  // El primer proceso comienza en 2
    } else {
        inicio = max(inicio, 2LL);
    }
    
    // El último proceso toma todos los números restantes
    if (rank == world_size - 1) {
        fin = N;
    } else {
        fin = (rank + 1) * bloque - 1;
    }
    
    // Cada proceso busca primos en su rango
    for (long long i = inicio; i <= fin; i++) {
        if (i == 2) {
            primos_locales.push_back(2);
        } else if (i % 2 != 0) {  // Saltar números pares
            if (esPrimo(i, primos_base)) {
                primos_locales.push_back(i);
            }
        }
    }
    
    return primos_locales;
}

// --------------------
// MAIN
// --------------------
int main(int argc, char** argv) {
    // Inicialización de MPI
    MPI_Init(&argc, &argv);
    
    // Obtener el número total de procesos
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Obtener el rank del proceso actual
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    long long N = 0;
    double tiempo_inicio, tiempo_fin;

    // Solo el proceso 0 lee la entrada y ejecuta la versión secuencial
    if (rank == 0) {
        cout << "Ingrese N: ";
        cin >> N;
        cout << "Procesando con " << world_size << " procesos MPI..." << endl;

        // ---- Secuencial (solo para comparación) ----
        auto t1 = chrono::high_resolution_clock::now();
        auto seq = primosSecuencial(N);
        auto t2 = chrono::high_resolution_clock::now();
        double tiempoSeq = chrono::duration<double>(t2 - t1).count();

        cout << "\n[Secuencial] " << seq.size() << " primos. Tiempo: "
             << tiempoSeq << " s\n";
        cout << "Primeros 10 primos: ";
        for (int i = 0; i < min(10, (int)seq.size()); i++)
            cout << seq[i] << " ";
        cout << "\nUltimos 10 primos: ";
        for (int i = max(0, (int)seq.size() - 10); i < seq.size(); i++)
            cout << seq[i] << " ";
        cout << "\n----------------------------------------\n";
    }

    // Broadcast: enviar N a todos los procesos
    MPI_Bcast(&N, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // Todos los procesos generan los primos base (hasta sqrt(N))
    // Estos primos son necesarios para verificar la primalidad
    long long limite_base = (long long)sqrt(N);
    vector<long long> primos_base = generarPrimosBase(limite_base);

    // Sincronizar antes de medir tiempo
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        tiempo_inicio = MPI_Wtime();
    }

    // ---- MPI: Cada proceso busca primos en su rango ----
    vector<long long> primos_locales = primosMPI(N, rank, world_size, primos_base);

    // Proceso 0 recolecta los resultados de todos los procesos
    if (rank == 0) {
        // Comenzar con los primos del proceso 0
        vector<long long> todos_primos = primos_locales;
        
        // Recibir primos de cada otro proceso
        for (int src = 1; src < world_size; src++) {
            // Primero recibir cuántos primos encontró ese proceso
            int count;
            MPI_Recv(&count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            if (count > 0) {
                // Recibir los primos
                vector<long long> primos_recibidos(count);
                MPI_Recv(&primos_recibidos[0], count, MPI_LONG_LONG, src, 1, 
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Agregar a la lista total
                todos_primos.insert(todos_primos.end(), 
                                   primos_recibidos.begin(), 
                                   primos_recibidos.end());
            }
        }
        
        // Ordenar todos los primos (paso 3)
        sort(todos_primos.begin(), todos_primos.end());
        
        tiempo_fin = MPI_Wtime();
        
        // Mostrar resultados
        cout << "\n[MPI con " << world_size << " procesos] " 
             << todos_primos.size() << " primos. Tiempo: "
             << (tiempo_fin - tiempo_inicio) << " s\n";
        
        cout << "Primeros 10 primos: ";
        for (int i = 0; i < min(10, (int)todos_primos.size()); i++)
            cout << todos_primos[i] << " ";
        
        cout << "\nUltimos 10 primos: ";
        for (int i = max(0, (int)todos_primos.size() - 10); 
             i < todos_primos.size(); i++)
            cout << todos_primos[i] << " ";
        cout << "\n";
        
    } else {
        // Los procesos no-maestro envían sus resultados al proceso 0
        int count = primos_locales.size();
        
        // Enviar la cantidad de primos encontrados
        MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        
        if (count > 0) {
            // Enviar los primos
            MPI_Send(&primos_locales[0], count, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
        }
    }

    // Finalización de MPI
    MPI_Finalize();
    
    return 0;
}
