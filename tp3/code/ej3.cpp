#include <mpi.h>
#include <bits/stdc++.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

static string detectar_ip_local() {
    int descriptor_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor_socket < 0) return "0.0.0.0";
    
    sockaddr_in direccion_destino{};
    direccion_destino.sin_family = AF_INET;
    direccion_destino.sin_port   = htons(53);
    ::inet_pton(AF_INET, "1.1.1.1", &direccion_destino.sin_addr);
    
    if (::connect(descriptor_socket, (sockaddr*)&direccion_destino, sizeof(direccion_destino)) < 0) { 
        ::close(descriptor_socket); 
        return "0.0.0.0"; 
    }
    
    sockaddr_in addr_local{}; 
    socklen_t tam_addr = sizeof(addr_local);
    if (::getsockname(descriptor_socket, (sockaddr*)&addr_local, &tam_addr) < 0) { 
        ::close(descriptor_socket); 
        return "0.0.0.0"; 
    }
    
    ::close(descriptor_socket);
    char buffer_ip[INET_ADDRSTRLEN] = {0};
    const char* cadena_ip = ::inet_ntop(AF_INET, &addr_local.sin_addr, buffer_ip, sizeof(buffer_ip));
    return cadena_ip ? string(cadena_ip) : "0.0.0.0";
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long long dimension_vectores = 100000000LL;
    
    if (rank == 0) {
        cout << "=== Producto Escalar de Vectores con MPI ===" << endl;
        cout << "Ingrese el tamaño de los vectores (Enter para usar " << dimension_vectores << "): ";
        long long entrada_dimension;
        if (cin >> entrada_dimension && entrada_dimension > 0) {
            dimension_vectores = entrada_dimension;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    MPI_Bcast(&dimension_vectores, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    string direccion_ip_proceso = detectar_ip_local();
    char nombre_procesador[MPI_MAX_PROCESSOR_NAME]; 
    int longitud_nombre = 0;
    MPI_Get_processor_name(nombre_procesador, &longitud_nombre);

    long long elementos_base = dimension_vectores / size;
    long long elementos_sobrantes  = dimension_vectores % size;
    long long indice_comienzo = rank * elementos_base + min<long long>(rank, elementos_sobrantes);
    long long cantidad_elementos  = elementos_base + (rank < elementos_sobrantes ? 1 : 0);
    long long indice_termino   = indice_comienzo + cantidad_elementos;

    vector<double> vector_A_local(cantidad_elementos);
    vector<double> vector_B_local(cantidad_elementos);
    
    for (long long idx = 0; idx < cantidad_elementos; ++idx) {
        long long posicion_global = indice_comienzo + idx;
        vector_A_local[idx] = (double)(posicion_global + 1);
        vector_B_local[idx] = (double)(dimension_vectores - posicion_global);
    }

    if (rank == 0) {
        cout << "Tamaño de vectores: " << dimension_vectores << endl;
        cout << "Número de procesos: " << size << endl;
        cout << "Elementos por proceso (aproximado): " << elementos_base << endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    timeval tiempo_inicio{}, tiempo_final{};
    if (rank == 0) gettimeofday(&tiempo_inicio, nullptr);

    double resultado_parcial = 0.0;
    for (long long idx = 0; idx < cantidad_elementos; ++idx) {
        resultado_parcial += vector_A_local[idx] * vector_B_local[idx];
    }

    double resultado_total = 0.0;
    MPI_Reduce(&resultado_parcial, &resultado_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    const int TAM_BUFFER_IP = 64;
    char buffer_mi_ip[TAM_BUFFER_IP]; 
    memset(buffer_mi_ip, 0, sizeof(buffer_mi_ip));
    snprintf(buffer_mi_ip, TAM_BUFFER_IP, "%s", direccion_ip_proceso.c_str());
    
    vector<char> ips_todos_procesos; 
    ips_todos_procesos.resize(size * TAM_BUFFER_IP, 0);
    MPI_Gather(buffer_mi_ip, TAM_BUFFER_IP, MPI_CHAR, ips_todos_procesos.data(), TAM_BUFFER_IP, MPI_CHAR, 0, MPI_COMM_WORLD);

    vector<long long> elementos_por_proceso(size);
    MPI_Gather(&cantidad_elementos, 1, MPI_LONG_LONG, elementos_por_proceso.data(), 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    vector<double> productos_parciales(size);
    MPI_Gather(&resultado_parcial, 1, MPI_DOUBLE, productos_parciales.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        gettimeofday(&tiempo_final, nullptr);
        double tiempo_ejecucion = (tiempo_final.tv_sec - tiempo_inicio.tv_sec) + 
                                  (tiempo_final.tv_usec - tiempo_inicio.tv_usec)/1e6;

        vector<string> mapeo_ip_por_rank(size);
        for (int proceso = 0; proceso < size; ++proceso) {
            mapeo_ip_por_rank[proceso] = string(&ips_todos_procesos[proceso * TAM_BUFFER_IP]);
        }

        cout << "\n=== Resultados ===" << endl;
        cout << fixed << setprecision(2);
        
        cout << "\nDistribución de trabajo:" << endl;
        for (int proceso = 0; proceso < size; ++proceso) {
            cout << "Proceso " << proceso << " (IP: " << mapeo_ip_por_rank[proceso] << ")" << endl;
            cout << "  - Elementos procesados: " << elementos_por_proceso[proceso] << endl;
            cout << "  - Producto parcial: " << scientific << productos_parciales[proceso] << endl;
        }

        cout << "\n=== Producto Escalar Final ===" << endl;
        cout << scientific << setprecision(10);
        cout << "A · B = " << resultado_total << endl;
        
        double valor_esperado = (double)dimension_vectores * (dimension_vectores + 1.0) * (dimension_vectores + 2.0) / 6.0;
        cout << "Valor esperado: " << valor_esperado << endl;
        double porcentaje_error = abs(resultado_total - valor_esperado) / valor_esperado * 100.0;
        cout << fixed << setprecision(6);
        cout << "Error relativo: " << porcentaje_error << "%" << endl;

        cout << "\n=== Tiempo de Ejecución ===" << endl;
        cout << "Tiempo total (MPI): " << tiempo_ejecucion << " segundos" << endl;
        
        double tiempo_secuencial_estimado = tiempo_ejecucion * size;
        cout << "Speedup estimado: " << (tiempo_secuencial_estimado / tiempo_ejecucion) << "x" << endl;
    }

    MPI_Finalize();
    return 0;
}

// Compilar: mpicxx -O3 -march=native -o ej3.out ej3.cpp
// Ejecutar local: mpirun -n 4 ./ej3.out
// Ejecutar en cluster: mpirun -n 8 --hostfile machinesfile.txt ./ej3.out