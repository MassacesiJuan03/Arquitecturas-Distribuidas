#!/bin/bash
# Script para desplegar y ejecutar ejercicios MPI en cluster
# Lee las IPs desde un archivo hosts.txt

# ===== CONFIGURACIÓN =====
HOSTS_FILE="hosts.txt"
USUARIO="massa"  # Cambia esto por tu usuario
REMOTE_DIR="~/tp3"
LOCAL_DIR="$(pwd)"

# Leer hosts desde archivo
if [ ! -f "$HOSTS_FILE" ]; then
    echo "ERROR: No se encuentra el archivo $HOSTS_FILE"
    echo "Crea un archivo hosts.txt con una IP por línea:"
    echo "  10.65.5.113"
    echo "  10.65.5.115"
    echo "  10.65.5.120"
    exit 1
fi

# Cargar hosts en array (ignorar líneas vacías y comentarios)
mapfile -t HOSTS < <(grep -v '^#' "$HOSTS_FILE" | grep -v '^[[:space:]]*$')

if [ ${#HOSTS[@]} -eq 0 ]; then
    echo "ERROR: El archivo $HOSTS_FILE está vacío o solo tiene comentarios"
    exit 1
fi

echo "================================================"
echo "  Despliegue de TP3 MPI en Cluster"
echo "================================================"
echo "Hosts detectados: ${#HOSTS[@]}"
for host in "${HOSTS[@]}"; do
    echo "  - $host"
done
echo ""

# ===== FUNCIÓN: Verificar SSH =====
verificar_ssh() {
    echo "Verificando conexión SSH a todos los hosts..."
    local errores=0
    for host in "${HOSTS[@]}"; do
        echo -n "  Conectando a $USUARIO@$host... "
        if ssh -o ConnectTimeout=5 -o BatchMode=yes "$USUARIO@$host" "echo 'OK'" &>/dev/null; then
            echo "✓"
        else
            echo "✗ ERROR"
            errores=$((errores + 1))
        fi
    done
    
    if [ $errores -gt 0 ]; then
        echo ""
        echo "ERROR: No se puede conectar a $errores host(s)."
        echo "Verifica:"
        echo "  1. SSH sin contraseña configurado (ssh-copy-id)"
        echo "  2. Las IPs son correctas"
        echo "  3. Los hosts están encendidos"
        exit 1
    fi
    echo "✓ Todas las conexiones SSH OK"
    echo ""
}

# ===== FUNCIÓN: Sincronizar código =====
sincronizar_codigo() {
    echo "Sincronizando código a todos los hosts..."
    for host in "${HOSTS[@]}"; do
        echo "  → $USUARIO@$host"
        ssh "$USUARIO@$host" "mkdir -p $REMOTE_DIR/code" &>/dev/null
        rsync -avz --quiet code/ "$USUARIO@$host:$REMOTE_DIR/code/" &
    done
    wait
    echo "✓ Código sincronizado en todos los hosts"
    echo ""
}

# ===== FUNCIÓN: Compilar en todos los hosts =====
compilar_remoto() {
    echo "Compilando en todos los hosts..."
    for host in "${HOSTS[@]}"; do
        echo "  Compilando en $host..."
        ssh "$USUARIO@$host" "cd $REMOTE_DIR/code && \
                   mpic++ -o ej1_mpi ej1.cpp -std=c++11 && \
                   mpic++ -o ej2_mpi ej2.cpp -std=c++11 && \
                   mpic++ -o ej3_mpi ej3.cpp -std=c++11 && \
                   mpic++ -o ej4_mpi ej4.cpp -std=c++11" &
    done
    wait
    echo "✓ Compilación completada en todos los hosts"
    echo ""
}

# ===== FUNCIÓN: Crear hostfile para MPI =====
crear_hostfile() {
    echo "Creando archivo hostfile para MPI..."
    
    # Detectar número de CPUs
    echo "# Archivo generado automáticamente desde $HOSTS_FILE" > hostfile
    echo "# Formato: hostname slots=numero_de_cpus" >> hostfile
    echo "" >> hostfile
    
    # Agregar localhost
    local_cpus=$(nproc 2>/dev/null || echo "2")
    echo "localhost slots=$local_cpus" >> hostfile
    
    # Agregar hosts remotos
    for host in "${HOSTS[@]}"; do
        # Intentar detectar número de CPUs en el host remoto
        cpus=$(ssh "$USUARIO@$host" "nproc" 2>/dev/null || echo "2")
        echo "$host slots=$cpus" >> hostfile
    done
    
    echo "✓ Archivo hostfile creado"
    echo ""
    echo "Contenido del hostfile:"
    cat hostfile
    echo ""
}

# ===== FUNCIÓN: Ejecutar ejercicio =====
ejecutar_ejercicio() {
    local ejercicio=$1
    local np=$2
    local input=$3
    
    echo "================================================"
    echo "  Ejecutando $ejercicio con $np procesos"
    echo "================================================"
    
    if [ -z "$input" ]; then
        mpirun --hostfile hostfile -np $np $REMOTE_DIR/code/$ejercicio
    else
        echo "$input" | mpirun --hostfile hostfile -np $np $REMOTE_DIR/code/$ejercicio
    fi
    echo ""
}

# ===== MENÚ PRINCIPAL =====
mostrar_menu() {
    echo "Selecciona una opción:"
    echo "1. Configurar y desplegar (primera vez)"
    echo "2. Solo sincronizar código"
    echo "3. Solo compilar"
    echo "4. Ejecutar Ejercicio 1 (Logaritmo)"
    echo "5. Ejecutar Ejercicio 3 (Matrices)"
    echo "6. Ejecutar Ejercicio 4 (Primos)"
    echo "7. Ejecutar todos los ejercicios"
    echo "0. Salir"
    echo ""
    read -p "Opción: " opcion
}

# ===== MAIN =====
case "$1" in
    --full)
        verificar_ssh
        sincronizar_codigo
        compilar_remoto
        crear_hostfile
        echo "✓ Configuración completa. Listo para ejecutar."
        ;;
    --sync)
        sincronizar_codigo
        ;;
    --compile)
        compilar_remoto
        ;;
    --run)
        shift
        ejercicio=$1
        np=${2:-8}
        ejecutar_ejercicio "$ejercicio" "$np" "$3"
        ;;
    *)
        mostrar_menu
        case $opcion in
            1)
                verificar_ssh
                sincronizar_codigo
                compilar_remoto
                crear_hostfile
                ;;
            2)
                sincronizar_codigo
                ;;
            3)
                compilar_remoto
                ;;
            4)
                ejecutar_ejercicio "ej1_mpi" 8
                ;;
            5)
                read -p "Tamaño de matriz: " size
                ejecutar_ejercicio "ej3_mpi" 8 "$size"
                ;;
            6)
                read -p "Valor de N: " n
                ejecutar_ejercicio "ej4_mpi" 8 "$n"
                ;;
            7)
                ejecutar_ejercicio "ej1_mpi" 8
                ejecutar_ejercicio "ej3_mpi" 8 "500"
                ejecutar_ejercicio "ej4_mpi" 8 "10000"
                ;;
            0)
                echo "Saliendo..."
                exit 0
                ;;
            *)
                echo "Opción no válida"
                ;;
        esac
        ;;
esac
