#include <mpi.h>
#include <bits/stdc++.h>
#include <sys/time.h>
using namespace std;

static long double calcular_serie_parcial(long double valor_y, long double valor_y_cuadrado,
                                          long long inicio, long long fin) {
    if (inicio >= fin) return 0.0L;
    
    long double potencia = valor_y * powl(valor_y_cuadrado, (long double)inicio);
    long double acumulador = 0.0L;
    
    for (long long indice = inicio; indice < fin; indice++) {
        long long denominador = 2LL * indice + 1LL;
        acumulador += potencia / (long double)denominador;
        potencia *= valor_y_cuadrado;
    }
    return acumulador;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);  // 

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long double valor_x = 1500000.0L; 
    long long cantidad_terminos = 10000000LL;
    
    if (rank == 0) {
        cerr << "Ingrese x (>=1500000) [Enter para usar 1500000]: ";
        long double entrada_x;
        if (cin >> entrada_x) valor_x = entrada_x;
        cin.clear(); 
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    MPI_Bcast(&valor_x, 1, MPI_LONG_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cantidad_terminos, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    if (valor_x < 1500000.0L) {
        if (rank == 0) {
            cerr << "x debe ser >= 1500000 (segun el TP)." << endl;
        }
        MPI_Finalize();
        return 1;
    }

    long double var_y  = (valor_x - 1.0L) / (valor_x + 1.0L);
    long double var_y_cuadrado = var_y * var_y;

    long long terminos_base = cantidad_terminos / size;
    long long resto  = cantidad_terminos % size;
    long long indice_inicial = rank * terminos_base + min<long long>(rank, resto);
    long long cantidad_a_tomar = terminos_base + (rank < resto ? 1 : 0);
    long long indice_final = indice_inicial + cantidad_a_tomar;

    MPI_Barrier(MPI_COMM_WORLD);
    timeval tiempo_inicio{}, tiempo_fin{};
    if (rank == 0) gettimeofday(&tiempo_inicio, nullptr);

    long double resultado_parcial = calcular_serie_parcial(var_y, var_y_cuadrado, indice_inicial, indice_final);

    long double total_global = 0.0L;
    MPI_Reduce(&resultado_parcial, &total_global, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        gettimeofday(&tiempo_fin, nullptr);
        double duracion = (tiempo_fin.tv_sec - tiempo_inicio.tv_sec) + (tiempo_fin.tv_usec - tiempo_inicio.tv_usec)/1e6;

        long double logaritmo_natural = 2.0L * total_global;
        cout << setprecision(15) << fixed;
        cout << "ln(x) = " << logaritmo_natural << "\n";
        cout << "Tiempo (s) = " << duracion << "\n";
    }

    MPI_Finalize();
    return 0;
}
// mpicxx -O3 -march=native -o ej1.out ej1.cpp 
// mpirun -n 8 ./ej1.out
// mpirun -n 32 --hostfile machinesfile.txt ./ej1.out