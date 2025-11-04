#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <mpi.h>      // Biblioteca MPI para programación paralela distribuida

using namespace std;
using namespace std::chrono;

// Calcula un elemento (row, col) del producto de matrices A * B
float prod_matrix(const vector<vector<float>>& A, const vector<vector<float>>& B, int row, int col) {
    float sum = 0.0f;
    int n = A[0].size();
    for (int k = 0; k < n; ++k) { 
        sum += A[row][k] * B[k][col];
    }
    return sum;
}

// Función para multiplicar un bloque de filas de A con toda la matriz B
// start_row: fila inicial del bloque
// end_row: fila final del bloque (exclusivo)
// Retorna el bloque resultante de la multiplicación
vector<vector<float>> multiply_rows(const vector<vector<float>>& A, 
                                     const vector<vector<float>>& B, 
                                     int start_row, int end_row, int N) {
    vector<vector<float>> result(end_row - start_row, vector<float>(N));
    
    for (int i = start_row; i < end_row; ++i) {
        for (int j = 0; j < N; ++j) {
            result[i - start_row][j] = prod_matrix(A, B, i, j);
        }
    }
    
    return result;
}

int main(int argc, char** argv){
    // Inicialización de MPI
    MPI_Init(&argc, &argv);
    
    // Obtener el número total de procesos
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Obtener el rank del proceso actual
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int N; // Tamaño de las matrices
    vector<vector<float>> A, B, C;
    double tiempo_inicio, tiempo_fin;

    // Solo el proceso 0 lee el tamaño e inicializa las matrices
    if (rank == 0) {
        cout << "Ingrese el tamaño de las matrices (N x N): ";
        cin >> N;
    }

    // Broadcast: enviar N a todos los procesos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Todos los procesos inicializan las matrices A y B
    A.resize(N, vector<float>(N));
    B.resize(N, vector<float>(N));
    
    if (rank == 0) {
        // Solo el proceso 0 inicializa los valores
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j) {
                A[i][j] = 0.1f;
                B[i][j] = 0.2f;
            }
    }

    // Broadcast: el proceso 0 envía las matrices A y B a todos los procesos
    // Cada proceso necesita ambas matrices completas para su cálculo
    for (int i = 0; i < N; ++i) {
        MPI_Bcast(&A[i][0], N, MPI_FLOAT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&B[i][0], N, MPI_FLOAT, 0, MPI_COMM_WORLD);
    }

    // Sincronizar antes de medir tiempo
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        tiempo_inicio = MPI_Wtime();
    }

    // Dividir las filas de la matriz resultado entre los procesos
    int rows_per_process = N / world_size;
    int start_row = rank * rows_per_process;
    int end_row;
    
    // El último proceso toma las filas restantes
    if (rank == world_size - 1) {
        end_row = N;
    } else {
        end_row = start_row + rows_per_process;
    }
    
    int local_rows = end_row - start_row;

    // Cada proceso calcula su bloque de filas de la matriz resultado
    vector<vector<float>> local_C = multiply_rows(A, B, start_row, end_row, N);

    // Preparar el resultado completo en el proceso 0
    if (rank == 0) {
        C.resize(N, vector<float>(N));
    }

    // MPI_Gather: recolectar todos los bloques de filas en el proceso 0
    // Necesitamos usar una estructura más simple para MPI_Gather
    // Convertir la matriz local a un vector unidimensional
    vector<float> local_flat(local_rows * N);
    for (int i = 0; i < local_rows; ++i) {
        for (int j = 0; j < N; ++j) {
            local_flat[i * N + j] = local_C[i][j];
        }
    }

    // Preparar los tamaños de recepción para MPI_Gatherv (ya que pueden ser diferentes)
    vector<int> recvcounts(world_size);
    vector<int> displs(world_size);
    
    if (rank == 0) {
        for (int i = 0; i < world_size; ++i) {
            int i_start = i * rows_per_process;
            int i_end = (i == world_size - 1) ? N : i_start + rows_per_process;
            recvcounts[i] = (i_end - i_start) * N;
            displs[i] = i_start * N;
        }
    }

    // Vector para recibir todos los datos en el proceso 0
    vector<float> C_flat;
    if (rank == 0) {
        C_flat.resize(N * N);
    }

    // MPI_Gatherv: recolectar datos de tamaños variables en el proceso 0
    MPI_Gatherv(&local_flat[0], local_rows * N, MPI_FLOAT,
                &C_flat[0], &recvcounts[0], &displs[0], MPI_FLOAT,
                0, MPI_COMM_WORLD);

    // Solo el proceso 0 muestra los resultados
    if (rank == 0) {
        tiempo_fin = MPI_Wtime();
        
        // Convertir el vector plano de vuelta a matriz
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                C[i][j] = C_flat[i * N + j];
            }
        }

        // Mostrar elementos específicos
        cout << "\n[MPI con " << world_size << " procesos]" << endl;
        cout << "Primer elemento C[0][0]: " << C[0][0] << endl;
        cout << "Elemento superior derecho C[0][" << N-1 << "]: " << C[0][N-1] << endl;
        cout << "Elemento inferior izquierdo C[" << N-1 << "][0]: " << C[N-1][0] << endl;
        cout << "Ultimo elemento C[" << N-1 << "][" << N-1 << "]: " << C[N-1][N-1] << endl;

        double tiempo_mpi = tiempo_fin - tiempo_inicio;
        cout << "\nTiempo de ejecución MPI: " << tiempo_mpi << " segundos\n";
    }

    // Finalización de MPI
    MPI_Finalize();
    
    return 0;
}