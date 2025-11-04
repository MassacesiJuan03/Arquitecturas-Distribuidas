#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>      // Biblioteca MPI para programación paralela distribuida
#include <vector>
#include <cmath>
#include <chrono>
#include <sys/time.h>

using namespace std;
using namespace std::chrono;

long long N = 10000000;

// Versión secuencial del cálculo de logaritmo mediante serie de Taylor
double log_taylor_whithout_threads(double x)
{
    double r = (x - 1) / (x + 1);
    double sum = 0.0;
    double pot = r; // r^n, comenzamos en n=0
    for (long long n = 0; n < N; n++)
    {
        sum += pot / (2 * n + 1);
        pot *= r * r; // r^(2n+2) para el siguiente término
    }

    return 2 * sum; // ln(x) = 2 * sum
    
}

// Función que calcula una porción de la serie de Taylor para MPI
// Cada proceso MPI ejecutará esta función con su rango específico [ini, fin]
double log_taylor_mpi(double x, long long ini, long long fin)
{
    double r = (x - 1) / (x + 1);
    double sum = 0.0;
    double pot = pow(r, ini); // Comenzamos en r^ini para este rango

    for (long long n = ini; n <= fin; n++)
    {
        sum += pot / (2 * n + 1);
        pot *= r * r; // r^(2n+2) para el siguiente término
    }
    return 2 * sum; // Resultado parcial de este proceso
}

int main(int argc, char** argv)
{
    // Inicialización de MPI - OBLIGATORIO al inicio del programa
    MPI_Init(&argc, &argv);
    
    // Obtener el número total de procesos MPI
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Obtener el identificador (rank) de este proceso
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    long double x = 1600000; // Valor de x para calcular ln(x)
    long double resultado_mpi = 0.0;
    double tiempo_inicio, tiempo_fin;
    
    // Debug: verificar que MPI se inicializó correctamente
    if (rank == 0) {
        cout << "\n=== Configuración MPI ===" << endl;
        cout << "Número de procesos detectados: " << world_size << endl;
        cout << "========================\n" << endl;
    }

    // Solo el proceso 0 (maestro) ejecuta la versión secuencial para comparar
    if (rank == 0) {
        cout << fixed;
        cout.precision(10);
        
        // ---------------------- SECUENCIAL ----------------------
        auto t1 = high_resolution_clock::now();
        
        long double resultado_secuencial = log_taylor_whithout_threads(x);
        
        auto t2 = high_resolution_clock::now();
        auto duracion_seq = duration_cast<milliseconds>(t2 - t1).count();
        
        cout << "\n[SECUENCIAL] ln(" << x << ") ≈ " << resultado_secuencial << endl;
        cout << "Tiempo secuencial: " << duracion_seq << " ms" << endl;
        cout << "----------------------------------------\n" << endl;
    }

    // Sincronizar todos los procesos antes de comenzar la versión paralela
    MPI_Barrier(MPI_COMM_WORLD);
    
    // ---------------------- PARALELO CON MPI ----------------------
    // El proceso 0 toma el tiempo de inicio
    if (rank == 0) {
        tiempo_inicio = MPI_Wtime(); // Función MPI para medir tiempo con alta precisión
    }
    
    // Calcular el tamaño del bloque que procesará cada proceso
    long long bloque = N / world_size;
    long long inicio = rank * bloque;
    long long fin;
    
    // El último proceso toma todos los elementos restantes
    if (rank == world_size - 1) {
        fin = N - 1;
    } else {
        fin = (rank + 1) * bloque - 1;
    }
    
    // Cada proceso calcula su parte de la serie de Taylor
    double resultado_local = log_taylor_mpi(x, inicio, fin);
    
    // MPI_Reduce: combina los resultados parciales de todos los procesos
    // - resultado_local: valor que envía cada proceso
    // - resultado_mpi_temp: donde se almacena la suma total (solo en proceso 0)
    // - 1: número de elementos a reducir
    // - MPI_DOUBLE: tipo de dato
    // - MPI_SUM: operación de reducción (suma)
    // - 0: proceso destino (rank 0)
    // - MPI_COMM_WORLD: comunicador
    double resultado_mpi_temp = 0.0;
    MPI_Reduce(&resultado_local, &resultado_mpi_temp, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    resultado_mpi = resultado_mpi_temp;
    
    // Solo el proceso 0 muestra los resultados finales
    if (rank == 0) {
        tiempo_fin = MPI_Wtime();
        double tiempo_mpi = (tiempo_fin - tiempo_inicio) * 1000.0; // Convertir a milisegundos
        
        cout << "[MPI con " << world_size << " procesos] ln(" << x << ") ≈ " << resultado_mpi << endl;
        cout << "Tiempo MPI: " << tiempo_mpi << " ms" << endl;
    }

    // Finalización de MPI - OBLIGATORIO al final del programa
    MPI_Finalize();
    
    return 0;
}