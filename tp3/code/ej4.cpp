#include <mpi.h>
#include <bits/stdc++.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

static string obtener_direccion_ip() {
    int socket_descriptor = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_descriptor < 0) return "0.0.0.0";
    
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    ::inet_pton(AF_INET, "1.1.1.1", &dest.sin_addr);
    
    if (::connect(socket_descriptor, (sockaddr*)&dest, sizeof(dest)) < 0) { 
        ::close(socket_descriptor); 
        return "0.0.0.0"; 
    }
    
    sockaddr_in direccion_local{}; 
    socklen_t longitud = sizeof(direccion_local);
    if (::getsockname(socket_descriptor, (sockaddr*)&direccion_local, &longitud) < 0) { 
        ::close(socket_descriptor); 
        return "0.0.0.0"; 
    }
    
    ::close(socket_descriptor);
    char buffer[INET_ADDRSTRLEN] = {0};
    const char* ip_string = ::inet_ntop(AF_INET, &direccion_local.sin_addr, buffer, sizeof(buffer));
    return ip_string ? string(ip_string) : "0.0.0.0";
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int tamano_matriz = 1000;
    
    if (rank == 0) {
        cout << "=== Multiplicacion de Matrices con MPI ===" << endl;
        cout << "Ingrese el tamaño NxN (Enter para usar " << tamano_matriz << "): ";
        int entrada_tamano;
        if (cin >> entrada_tamano && entrada_tamano > 0) {
            tamano_matriz = entrada_tamano;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    MPI_Bcast(&tamano_matriz, 1, MPI_INT, 0, MPI_COMM_WORLD);

    string ip_actual = obtener_direccion_ip();
    char nombre_nodo[MPI_MAX_PROCESSOR_NAME]; 
    int longitud_nombre_nodo = 0;
    MPI_Get_processor_name(nombre_nodo, &longitud_nombre_nodo);

    int filas_base = tamano_matriz / size;
    int filas_adicionales = tamano_matriz % size;
    int filas_asignadas = filas_base + (rank < filas_adicionales ? 1 : 0);
    int fila_inicial = rank * filas_base + min(rank, filas_adicionales);

    vector<double> matriz_A_local(filas_asignadas * tamano_matriz);
    vector<double> matriz_B(tamano_matriz * tamano_matriz);
    vector<double> matriz_C_local(filas_asignadas * tamano_matriz);

    if (rank == 0) {
        vector<double> matriz_A_completa(tamano_matriz * tamano_matriz);
        for (int idx = 0; idx < tamano_matriz * tamano_matriz; ++idx) {
            matriz_A_completa[idx] = (double)(idx % 100);
        }
        for (int idx = 0; idx < tamano_matriz * tamano_matriz; ++idx) {
            matriz_B[idx] = (double)((idx * 2) % 100);
        }

        int desplazamiento = 0;
        for (int proceso = 0; proceso < size; ++proceso) {
            int filas_proceso = filas_base + (proceso < filas_adicionales ? 1 : 0);
            if (proceso == 0) {
                memcpy(matriz_A_local.data(), matriz_A_completa.data(), filas_asignadas * tamano_matriz * sizeof(double));
            } else {
                MPI_Send(matriz_A_completa.data() + desplazamiento, filas_proceso * tamano_matriz, MPI_DOUBLE, proceso, 0, MPI_COMM_WORLD);
            }
            desplazamiento += filas_proceso * tamano_matriz;
        }
    } else {
        MPI_Recv(matriz_A_local.data(), filas_asignadas * tamano_matriz, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Bcast(matriz_B.data(), tamano_matriz * tamano_matriz, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "Tamaño de matrices: " << tamano_matriz << "x" << tamano_matriz << endl;
        cout << "Número de procesos: " << size << endl;
        cout << "Filas por proceso: " << filas_base << " (+" << filas_adicionales << " extra)" << endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    timeval inicio{}, fin{};
    if (rank == 0) gettimeofday(&inicio, nullptr);

    for (int fila = 0; fila < filas_asignadas; ++fila) {
        for (int columna = 0; columna < tamano_matriz; ++columna) {
            double suma_productos = 0.0;
            for (int indice_k = 0; indice_k < tamano_matriz; ++indice_k) {
                suma_productos += matriz_A_local[fila * tamano_matriz + indice_k] * matriz_B[indice_k * tamano_matriz + columna];
            }
            matriz_C_local[fila * tamano_matriz + columna] = suma_productos;
        }
    }

    vector<double> matriz_resultado;
    if (rank == 0) {
        matriz_resultado.resize(tamano_matriz * tamano_matriz);
        memcpy(matriz_resultado.data(), matriz_C_local.data(), filas_asignadas * tamano_matriz * sizeof(double));
        
        int posicion = filas_asignadas * tamano_matriz;
        for (int proc = 1; proc < size; ++proc) {
            int filas_proc = filas_base + (proc < filas_adicionales ? 1 : 0);
            MPI_Recv(matriz_resultado.data() + posicion, filas_proc * tamano_matriz, MPI_DOUBLE, proc, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            posicion += filas_proc * tamano_matriz;
        }
    } else {
        MPI_Send(matriz_C_local.data(), filas_asignadas * tamano_matriz, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }

    const int TAMANO_CADENA_IP = 64;
    char buffer_ip_local[TAMANO_CADENA_IP]; 
    memset(buffer_ip_local, 0, sizeof(buffer_ip_local));
    snprintf(buffer_ip_local, TAMANO_CADENA_IP, "%s", ip_actual.c_str());
    
    vector<char> buffer_todas_ips; 
    buffer_todas_ips.resize(size * TAMANO_CADENA_IP, 0);
    MPI_Gather(buffer_ip_local, TAMANO_CADENA_IP, MPI_CHAR, buffer_todas_ips.data(), TAMANO_CADENA_IP, MPI_CHAR, 0, MPI_COMM_WORLD);

    vector<int> conteo_filas(size);
    MPI_Gather(&filas_asignadas, 1, MPI_INT, conteo_filas.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        gettimeofday(&fin, nullptr);
        double tiempo_transcurrido = (fin.tv_sec - inicio.tv_sec) + (fin.tv_usec - inicio.tv_usec)/1e6;

        vector<string> ips_procesos(size);
        for (int id_proceso = 0; id_proceso < size; ++id_proceso) {
            ips_procesos[id_proceso] = string(&buffer_todas_ips[id_proceso * TAMANO_CADENA_IP]);
        }

        cout << "\n=== Resultados ===" << endl;
        cout << "\nDistribución de trabajo:" << endl;
        for (int id_proceso = 0; id_proceso < size; ++id_proceso) {
            cout << "Proceso " << id_proceso << " (IP: " << ips_procesos[id_proceso] << ")" << endl;
            cout << "  - Filas procesadas: " << conteo_filas[id_proceso] << endl;
        }

        double suma_total = 0.0;
        for (int idx = 0; idx < tamano_matriz * tamano_matriz; ++idx) {
            suma_total += matriz_resultado[idx];
        }

        cout << "\n=== Resultado de C = A x B ===" << endl;
        cout << fixed << setprecision(2);
        cout << "Esquina superior izquierda: " << matriz_resultado[0] << endl;
        cout << "Esquina superior derecha: " << matriz_resultado[tamano_matriz-1] << endl;
        cout << "Esquina inferior izquierda: " << matriz_resultado[(tamano_matriz-1)*tamano_matriz] << endl;
        cout << "Esquina inferior derecha: " << matriz_resultado[tamano_matriz*tamano_matriz-1] << endl;
        cout << scientific << setprecision(6);
        cout << "Sumatoria total de C: " << suma_total << endl;

        cout << "\n=== Tiempo de Ejecución ===" << endl;
        cout << "Tiempo total (MPI): " << tiempo_transcurrido << " segundos" << endl;
    }

    MPI_Finalize();
    return 0;
}

// Compilar: mpicxx -O3 -march=native -o ej4.out ej4.cpp
// Ejecutar local: mpirun -n 4 ./ej4.out
// Ejecutar en cluster: mpirun -n 8 --hostfile machinesfile.txt ./ej4.out