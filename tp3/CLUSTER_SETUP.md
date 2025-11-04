# Guía de Configuración de MPI en Cluster

## 🖥️ Configuración de Cluster con MPI

Esta guía te ayudará a ejecutar los ejercicios MPI en múltiples computadoras conectadas por red.

## 📋 Requisitos

### En TODAS las máquinas:
1. **Linux** (Ubuntu, Debian, CentOS, etc.)
2. **Mismo usuario** con el mismo nombre
3. **MPI instalado** (Open MPI recomendado)
4. **Conectividad de red** (ping entre máquinas)
5. **SSH habilitado**

## 🔧 Paso 1: Instalar MPI en Todas las Máquinas

```bash
# En cada máquina del cluster
sudo apt-get update
sudo apt-get install openmpi-bin openmpi-common libopenmpi-dev

# Verificar instalación
mpirun --version
```

## 🔑 Paso 2: Configurar SSH sin Contraseña

### En la máquina principal (desde donde ejecutarás mpirun):

```bash
# Generar par de claves SSH (si no existe)
ssh-keygen -t rsa -b 4096
# Presiona Enter para todas las opciones (sin contraseña)

# Copiar clave pública a cada máquina remota
ssh-copy-id usuario@192.168.1.101
ssh-copy-id usuario@192.168.1.102
ssh-copy-id usuario@192.168.1.103

# Probar conexión sin contraseña
ssh usuario@192.168.1.101 "hostname"
```

Si `ssh-copy-id` no funciona:
```bash
cat ~/.ssh/id_rsa.pub | ssh usuario@192.168.1.101 "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"
```

## 📁 Paso 3: Crear Archivo de Hosts

Crea `hostfile` en el directorio del proyecto:

```bash
cd tp3
nano hostfile
```

**Opción A - Usando hostnames:**
```
localhost slots=4
pc1 slots=4
pc2 slots=4
pc3 slots=4
```

**Opción B - Usando IPs:**
```
192.168.1.100 slots=4
192.168.1.101 slots=4
192.168.1.102 slots=4
192.168.1.103 slots=4
```

**Opción C - Distribuir específicamente:**
```
# Máquina principal: 2 procesos
localhost slots=2 max_slots=2

# Máquinas remotas: 2 procesos cada una
192.168.1.101 slots=2 max_slots=2
192.168.1.102 slots=2 max_slots=2
192.168.1.103 slots=2 max_slots=2
```

**Nota:** `slots` = número de núcleos/procesos a usar en esa máquina

## 🚀 Paso 4: Desplegar el Código

### Opción A - Usando el Script Automático:

```bash
# Editar el script con tus hosts
nano deploy_cluster.sh

# Modificar estas líneas:
HOSTS=("usuario@192.168.1.101" "usuario@192.168.1.102" "usuario@192.168.1.103")

# Dar permisos de ejecución
chmod +x deploy_cluster.sh

# Ejecutar despliegue completo
./deploy_cluster.sh --full
```

### Opción B - Manualmente:

```bash
# Crear directorio en máquinas remotas
for host in usuario@192.168.1.101 usuario@192.168.1.102 usuario@192.168.1.103; do
    ssh $host "mkdir -p ~/tp3/code"
done

# Copiar código a todas las máquinas
for host in usuario@192.168.1.101 usuario@192.168.1.102 usuario@192.168.1.103; do
    rsync -avz code/ $host:~/tp3/code/
done

# Compilar en todas las máquinas
for host in usuario@192.168.1.101 usuario@192.168.1.102 usuario@192.168.1.103; do
    ssh $host "cd ~/tp3/code && mpic++ -o ej1_mpi ej1.cpp -std=c++11"
    ssh $host "cd ~/tp3/code && mpic++ -o ej2_mpi ej2.cpp -std=c++11"
    ssh $host "cd ~/tp3/code && mpic++ -o ej3_mpi ej3.cpp -std=c++11"
    ssh $host "cd ~/tp3/code && mpic++ -o ej4_mpi ej4.cpp -std=c++11"
done
```

## ▶️ Paso 5: Ejecutar en el Cluster

```bash
# Ejecutar con archivo hostfile
mpirun --hostfile hostfile -np 8 ~/tp3/code/ej1_mpi

# Ejecutar especificando hosts directamente
mpirun -np 8 --host 192.168.1.101,192.168.1.102,192.168.1.103,localhost ~/tp3/code/ej1_mpi

# Ejercicio 3 (con input)
echo 1000 | mpirun --hostfile hostfile -np 8 ~/tp3/code/ej3_mpi

# Ejercicio 4 (con input)
echo 100000 | mpirun --hostfile hostfile -np 8 ~/tp3/code/ej4_mpi
```

## 📊 Ejemplos de Ejecución

### Ejemplo 1: 8 procesos distribuidos en 4 máquinas (2 por máquina)
```bash
mpirun --hostfile hostfile -np 8 ~/tp3/code/ej1_mpi
```

### Ejemplo 2: 16 procesos distribuidos (4 por máquina)
```bash
mpirun --hostfile hostfile -np 16 ~/tp3/code/ej4_mpi
```

### Ejemplo 3: Especificar cuántos procesos por host
```bash
mpirun -np 12 \
    --host localhost:4,pc1:4,pc2:4 \
    ~/tp3/code/ej3_mpi
```

## 🔍 Verificación y Debugging

### 1. Verificar conectividad
```bash
# Probar SSH a cada máquina
for host in pc1 pc2 pc3; do
    echo -n "Testing $host: "
    ssh $host "hostname" && echo "✓" || echo "✗"
done
```

### 2. Probar MPI básico
```bash
# Crear programa de prueba
cat > test_mpi.cpp << 'EOF'
#include <mpi.h>
#include <iostream>
#include <unistd.h>
using namespace std;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    char hostname[256];
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    gethostname(hostname, sizeof(hostname));
    
    cout << "Proceso " << rank << " de " << size 
         << " ejecutándose en " << hostname << endl;
    
    MPI_Finalize();
    return 0;
}
EOF

# Compilar
mpic++ -o test_mpi test_mpi.cpp

# Copiar a otras máquinas
for host in usuario@pc1 usuario@pc2 usuario@pc3; do
    scp test_mpi $host:~/
done

# Ejecutar
mpirun --hostfile hostfile -np 8 ~/test_mpi
```

**Salida esperada:**
```
Proceso 0 de 8 ejecutándose en localhost
Proceso 1 de 8 ejecutándose en localhost
Proceso 2 de 8 ejecutándose en pc1
Proceso 3 de 8 ejecutándose en pc1
Proceso 4 de 8 ejecutándose en pc2
Proceso 5 de 8 ejecutándose en pc2
Proceso 6 de 8 ejecutándose en pc3
Proceso 7 de 8 ejecutándose en pc3
```

### 3. Variables de entorno útiles
```bash
# Mostrar más información de debug
export OMPI_MCA_plm_base_verbose=100

# Si hay problemas con shared memory en WSL/VM
export OMPI_MCA_btl_vader_single_copy_mechanism=none

# Agregar a ~/.bashrc para hacerlo permanente
echo 'export OMPI_MCA_btl_vader_single_copy_mechanism=none' >> ~/.bashrc
```

## ❌ Solución de Problemas

### Error: "Permission denied"
```bash
# Verificar permisos de ~/.ssh
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

### Error: "Host key verification failed"
```bash
# Agregar hosts a known_hosts
for host in pc1 pc2 pc3; do
    ssh-keyscan -H $host >> ~/.ssh/known_hosts
done
```

### Error: "mpirun cannot find executable"
```bash
# Usar rutas absolutas
mpirun --hostfile hostfile -np 8 /home/usuario/tp3/code/ej1_mpi
```

### Error: Versiones diferentes de MPI
```bash
# Verificar versión en cada máquina
for host in pc1 pc2 pc3; do
    echo "$host:"
    ssh $host "mpirun --version"
done

# Instalar la misma versión en todas
```

## 📈 Optimización de Rendimiento

### 1. Usar binding de CPU
```bash
# Asignar procesos a núcleos específicos
mpirun --bind-to core --hostfile hostfile -np 8 ~/tp3/code/ej1_mpi
```

### 2. Configurar timeout
```bash
# Timeout de 5 minutos
mpirun --timeout 300 --hostfile hostfile -np 8 ~/tp3/code/ej4_mpi
```

### 3. Mapeo personalizado
```bash
# 4 procesos en pc1, 2 en pc2, 2 en pc3
mpirun --host pc1:4,pc2:2,pc3:2 -np 8 ~/tp3/code/ej1_mpi
```

## 🌐 Configuración Avanzada: NFS (Opcional)

Si quieres que todas las máquinas vean la misma carpeta:

```bash
# En el servidor (máquina principal)
sudo apt-get install nfs-kernel-server
sudo mkdir -p /export/tp3
sudo cp -r ~/tp3/* /export/tp3/

# Editar /etc/exports
echo "/export/tp3 192.168.1.0/24(rw,sync,no_subtree_check)" | sudo tee -a /etc/exports

sudo exportfs -a
sudo systemctl restart nfs-kernel-server

# En cada cliente
sudo apt-get install nfs-common
sudo mkdir -p /mnt/tp3
sudo mount 192.168.1.100:/export/tp3 /mnt/tp3

# Agregar a /etc/fstab para montaje automático
echo "192.168.1.100:/export/tp3 /mnt/tp3 nfs defaults 0 0" | sudo tee -a /etc/fstab
```

## 📝 Checklist Final

- [ ] MPI instalado en todas las máquinas
- [ ] SSH sin contraseña configurado
- [ ] Archivo hostfile creado
- [ ] Código copiado a todas las máquinas
- [ ] Código compilado en todas las máquinas
- [ ] Test básico de MPI funciona
- [ ] Rutas absolutas en comandos mpirun

¡Listo para ejecutar en cluster! 🚀
