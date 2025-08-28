#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

float prod_matrix(const vector<vector<float>>& A, const vector<vector<float>>& B, int row, int col) {
    float sum = 0.0f;
    int n = A[0].size();
    for (int k = 0; k < n; ++k) { 
        sum += A[row][k] * B[k][col];
    }
    return sum;
}

int main(){
    int N; // Tamaño de las matrices
    int num_threads = 10; // Número de hilos

    cout << "Ingrese el tamaño de las matrices (N x N): ";
    cin >> N;

    // Inicializar matrices A y B con valores aleatorios
    vector<vector<float>> A(N, vector<float>(N));
    vector<vector<float>> B(N, vector<float>(N));
    float sumatoria = 0.0f;

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            A[i][j] = 0.1;
            B[i][j] = 0.2;
        }

    // Medir tiempo de ejecución secuencial
    auto start_seq = high_resolution_clock::now();

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j){
            if (j==0 && i==0){
                float value = prod_matrix(A, B, i, j);
                cout << "Primer elemento: " << value << endl;
                sumatoria += value;
            }else if ( i==N-1 && j==N-1 ){
                float value = prod_matrix(A, B, i, j);
                cout << "Ultimo elemento: " << value << endl;
                sumatoria += value;
            }else if (i==0 && j==N-1){
                float value = prod_matrix(A, B, i, j);
                cout << "Elemento superior derecho: " << value << endl;
                sumatoria += value;
            }else if (i==N-1 && j==0){
                float value = prod_matrix(A, B, i, j);
                cout << "Elemento inferior izquierdo: " << value << endl;
                sumatoria += value;
            }else{
                sumatoria += prod_matrix(A, B, i, j);
            }
        }  

    auto end_seq = high_resolution_clock::now();
    double elapsed_sec = duration<double>(end_seq - start_seq).count();

    cout << "Tiempo de ejecución secuencial: " << elapsed_sec << " segundos\n";
    cout << "----------------------------------------\n" << endl;

    // Medir tiempo de ejecución paralelo
    sumatoria = 0.0f;
    auto start_par = high_resolution_clock::now();

    vector<thread> workers;
    int rows_per_thread = N / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int start_row = t * rows_per_thread;
        int end_row = (t == num_threads - 1) ? N : start_row + rows_per_thread;

        workers.emplace_back([&, start_row, end_row]() {
            for (int i = start_row; i < end_row; ++i)
                for (int j = 0; j < N; ++j){
                    if (j==0 && i==0){
                        float value = prod_matrix(A, B, i, j);
                        cout << "Primer elemento: " << value << endl;
                        sumatoria += value;
                    }else if ( i==N-1 && j==N-1 ){
                        float value = prod_matrix(A, B, i, j);
                        cout << "Ultimo elemento: " << value << endl;
                        sumatoria += value;
                    }else if (i==0 && j==N-1){
                        float value = prod_matrix(A, B, i, j);
                        cout << "Elemento superior derecho: " << value << endl;
                        sumatoria += value;
                    }else if (i==N-1 && j==0){
                        float value = prod_matrix(A, B, i, j);
                        cout << "Elemento inferior izquierdo: " << value << endl;
                        sumatoria += value;
                    }else{
                        sumatoria += prod_matrix(A, B, i, j);
                    }
                }
        });
    }

    for (auto& th : workers) th.join();

    auto end_par = high_resolution_clock::now();

    double elapsed_par = duration<double>(end_par - start_par).count();
    cout << "Tiempo de ejecución paralelo: " << elapsed_par << " segundos\n";

    //Speedup
    cout << "Speedup: " << elapsed_sec / elapsed_par << endl;

}