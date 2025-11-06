#include <mpi.h>
#include <bits/stdc++.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

// ---------------------- Funciones auxiliares ----------------------

static string conseguir_direccion_ip() {
    int socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) return "0.0.0.0";
    
    sockaddr_in destino{};
    destino.sin_family = AF_INET;
    destino.sin_port = htons(53);
    ::inet_pton(AF_INET, "1.1.1.1", &destino.sin_addr);
    
    if (::connect(socket_fd, (sockaddr*)&destino, sizeof(destino)) < 0) {
        ::close(socket_fd);
        return "0.0.0.0";
    }
    
    sockaddr_in local_addr{};
    socklen_t longitud = sizeof(local_addr);
    if (::getsockname(socket_fd, (sockaddr*)&local_addr, &longitud) < 0) {
        ::close(socket_fd);
        return "0.0.0.0";
    }
    
    ::close(socket_fd);
    char buffer[INET_ADDRSTRLEN] = {0};
    const char* direccion_ip = ::inet_ntop(AF_INET, &local_addr.sin_addr, buffer, sizeof(buffer));
    return direccion_ip ? string(direccion_ip) : "0.0.0.0";
}

static bool cargar_contenido_archivo(const string& ruta_archivo, string& contenido) {
    ifstream archivo(ruta_archivo, ios::in | ios::binary);
    if (!archivo) return false;
    
    archivo.seekg(0, ios::end);
    size_t tamanio = (size_t)archivo.tellg();
    contenido.resize(tamanio);
    archivo.seekg(0, ios::beg);
    
    if (tamanio > 0) archivo.read(&contenido[0], tamanio);
    return true;
}

static bool cargar_patrones_desde_archivo(const string& ruta, vector<string>& lista_patrones) {
    ifstream archivo(ruta);
    if (!archivo) return false;
    
    string linea;
    while (getline(archivo, linea)) {
        while (!linea.empty() && (linea.back()=='\r' || linea.back()=='\n')) 
            linea.pop_back();
        lista_patrones.push_back(linea);
    }
    return true;
}

static int contar_ocurrencias_con_solapamiento(const string& texto_completo, const string& patron) {
    if (patron.empty()) return 0;
    
    int contador = 0;
    size_t posicion = 0;
    
    while ((posicion = texto_completo.find(patron, posicion)) != string::npos) {
        contador++;
        posicion++;
    }
    return contador;
}

// ---------------------- Programa principal ----------------------

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    string contenido_texto;
    vector<string> lista_patrones;
    bool carga_texto_exitosa = cargar_contenido_archivo("texto.txt", contenido_texto);
    bool carga_patrones_exitosa = cargar_patrones_desde_archivo("patrones.txt", lista_patrones);

    int estado_local = (carga_texto_exitosa && carga_patrones_exitosa) ? 1 : 0;
    int estado_global = 0;
    MPI_Allreduce(&estado_local, &estado_global, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    
    if (!estado_global) {
        if (rank == 0) {
            cerr << "Error: no se pudo leer texto.txt o patrones.txt en todos los procesos\n";
        }
        MPI_Finalize();
        return 1;
    }

    const int total_patrones = (int)lista_patrones.size();
    int patrones_por_proceso = total_patrones / size;
    int patrones_restantes = total_patrones % size;
    int indice_inicio = rank * patrones_por_proceso + min(rank, patrones_restantes);
    int cantidad_patrones = patrones_por_proceso + (rank < patrones_restantes ? 1 : 0);
    int indice_fin = indice_inicio + cantidad_patrones;

    string mi_direccion_ip = conseguir_direccion_ip();
    char nombre_host[MPI_MAX_PROCESSOR_NAME];
    int longitud_nombre = 0;
    MPI_Get_processor_name(nombre_host, &longitud_nombre);

    MPI_Barrier(MPI_COMM_WORLD);
    timeval inicio_tiempo{}, fin_tiempo{};
    if (rank == 0) gettimeofday(&inicio_tiempo, nullptr);

    vector<int> conteos_locales(total_patrones, -1);
    vector<int> propietarios_locales(total_patrones, -1);
    
    for (int idx = indice_inicio; idx < indice_fin; ++idx) {
        int cantidad = contar_ocurrencias_con_solapamiento(contenido_texto, lista_patrones[idx]);
        conteos_locales[idx] = cantidad;
        propietarios_locales[idx] = rank;
    }

    vector<int> conteos_globales(total_patrones, 0);
    vector<int> propietarios_globales(total_patrones, 0);
    MPI_Reduce(conteos_locales.data(), conteos_globales.data(), total_patrones, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(propietarios_locales.data(), propietarios_globales.data(), total_patrones, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    const int LONGITUD_IP = 64;
    char buffer_ip[LONGITUD_IP];
    memset(buffer_ip, 0, sizeof(buffer_ip));
    snprintf(buffer_ip, LONGITUD_IP, "%s", mi_direccion_ip.c_str());
    
    vector<char> todas_las_ips;
    todas_las_ips.resize(size * LONGITUD_IP, 0);
    MPI_Gather(buffer_ip, LONGITUD_IP, MPI_CHAR, todas_las_ips.data(), LONGITUD_IP, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        gettimeofday(&fin_tiempo, nullptr);
        double duracion_total = (fin_tiempo.tv_sec - inicio_tiempo.tv_sec) + 
                               (fin_tiempo.tv_usec - inicio_tiempo.tv_usec)/1e6;

        vector<string> mapa_rank_ip(size);
        for (int proceso = 0; proceso < size; ++proceso) {
            mapa_rank_ip[proceso] = string(&todas_las_ips[proceso * LONGITUD_IP]);
        }

        for (int indice = 0; indice < total_patrones; ++indice) {
            int conteo = conteos_globales[indice] < 0 ? 0 : conteos_globales[indice];
            int propietario = propietarios_globales[indice];
            string ip_procesador = (propietario >= 0 && propietario < size) ? 
                                   mapa_rank_ip[propietario] : "0.0.0.0";
            cout << "el patron " << indice << " aparece " << conteo 
                 << " veces. Buscado por " << ip_procesador << "\n";
        }

        cout << fixed << setprecision(6);
        cout << "Tiempo de ejecucion (MPI): " << duracion_total << " segundos\n";
    }

    MPI_Finalize();
    return 0;