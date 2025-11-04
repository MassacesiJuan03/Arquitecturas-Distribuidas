# TP3 - Implementación con MPI (Message Passing Interface)

## Descripción

Este trabajo práctico implementa 4 ejercicios utilizando MPI para programación paralela distribuida:

1. **Ejercicio 1**: Cálculo de logaritmo natural mediante serie de Taylor
2. **Ejercicio 2**: Búsqueda de patrones en texto usando algoritmo Rabin-Karp
3. **Ejercicio 3**: Multiplicación de matrices
4. **Ejercicio 4**: Búsqueda de números primos usando criba optimizada

## Requisitos

- Compilador C++ (g++)
- Open MPI o MPICH instalado
- Sistema operativo: Linux/Unix (recomendado) o Windows con WSL

### Instalación de MPI en Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install mpich
# O alternativamente:
sudo apt-get install openmpi-bin openmpi-common libopenmpi-dev
```

## Compilación

### Ejercicio 1 - Logaritmo con Serie de Taylor
```bash
cd code
mpic++ -o ej1_mpi ej1.cpp -std=c++11
```

### Ejercicio 2 - Rabin-Karp
```bash
cd code
mpic++ -o ej2_mpi ej2.cpp -std=c++11
```

### Ejercicio 3 - Multiplicación de Matrices
```bash
cd code
mpic++ -o ej3_mpi ej3.cpp -std=c++11
```

### Ejercicio 4 - Números Primos
```bash
cd code
mpic++ -o ej4_mpi ej4.cpp -std=c++11
```

## Ejecución

La sintaxis general para ejecutar programas MPI es:
```bash
mpirun -np <número_de_procesos> ./programa
```

### Ejercicio 1
```bash
# Ejecutar con 4 procesos
mpirun -np 4 ./ej1_mpi

# Ejecutar con 8 procesos
mpirun -np 8 ./ej1_mpi
```

### Ejercicio 2
**Nota**: Asegúrate de tener los archivos `texto.txt` y `patrones.txt` en el directorio correcto.

```bash
# Ejecutar con 4 procesos
mpirun -np 4 ./ej2_mpi

# Ejecutar con 6 procesos
mpirun -np 6 ./ej2_mpi
```

### Ejercicio 3
```bash
# Ejecutar con 4 procesos
mpirun -np 4 ./ej3_mpi
# Ingresar el tamaño de la matriz cuando se solicite (ejemplo: 1000)

# Ejecutar con 8 procesos
mpirun -np 8 ./ej3_mpi
```

### Ejercicio 4
```bash
# Ejecutar con 4 procesos
mpirun -np 4 ./ej4_mpi
# Ingresar N cuando se solicite (ejemplo: 100000)

# Ejecutar con 8 procesos
mpirun -np 8 ./ej4_mpi
```

## Conceptos MPI Utilizados

### 1. Inicialización y Finalización
- `MPI_Init()`: Inicializa el entorno MPI (obligatorio al inicio)
- `MPI_Finalize()`: Finaliza el entorno MPI (obligatorio al final)
- `MPI_Comm_size()`: Obtiene el número total de procesos
- `MPI_Comm_rank()`: Obtiene el identificador (rank) del proceso actual

### 2. Comunicación Colectiva
- `MPI_Bcast()`: Broadcast - el proceso maestro envía datos a todos los demás
- `MPI_Reduce()`: Reduce - combina datos de todos los procesos usando una operación (suma, max, etc.)
- `MPI_Gather()`: Recolecta datos de todos los procesos en el proceso maestro
- `MPI_Gatherv()`: Similar a Gather pero permite tamaños variables de datos
- `MPI_Barrier()`: Sincroniza todos los procesos (punto de encuentro)

### 3. Comunicación Punto a Punto
- `MPI_Send()`: Envía datos de un proceso a otro específico
- `MPI_Recv()`: Recibe datos de un proceso específico

### 4. Medición de Tiempo
- `MPI_Wtime()`: Retorna el tiempo transcurrido con alta precisión

## Patrones de Paralelización Utilizados

### Ejercicio 1 (Logaritmo)
- **Patrón**: División de datos (Data Decomposition)
- Cada proceso calcula una porción de la serie de Taylor
- Se usa `MPI_Reduce` con operación `MPI_SUM` para combinar resultados parciales

### Ejercicio 2 (Rabin-Karp)
- **Patrón**: División de tareas (Task Decomposition)
- Cada proceso busca un subconjunto de patrones en el texto completo
- Se distribuyen los patrones entre procesos, cada uno tiene una copia del texto

### Ejercicio 3 (Multiplicación de Matrices)
- **Patrón**: División de datos por filas
- Cada proceso calcula un bloque de filas de la matriz resultado
- Se usa `MPI_Gatherv` para recolectar los bloques de resultado

### Ejercicio 4 (Números Primos)
- **Patrón**: División de datos (Data Decomposition)
- Cada proceso verifica la primalidad de un rango de números
- Los primos base se calculan en todos los procesos
- Se usa comunicación punto a punto para recolectar los primos encontrados

## Ventajas de MPI

1. **Escalabilidad**: Puede ejecutarse en múltiples nodos de un cluster
2. **Memoria Distribuida**: Cada proceso tiene su propio espacio de memoria
3. **Portabilidad**: Funciona en diferentes arquitecturas y sistemas operativos
4. **Rendimiento**: Ideal para problemas que requieren gran cantidad de cómputo

## Notas Importantes

- El número de procesos debe ser apropiado para el problema
- Para ejercicios 1 y 3, mejor si el número de procesos divide bien el tamaño del problema
- En sistemas con múltiples núcleos, usar `-np` igual o menor al número de núcleos disponibles
- Para ejecutar en múltiples máquinas, se requiere configuración adicional de MPI

## Troubleshooting

### Error: "mpirun not found"
Asegúrate de tener MPI instalado correctamente.

### Error en tiempo de ejecución
Verifica que el número de procesos sea apropiado para tu sistema.

### Archivos no encontrados (Ejercicio 2)
Ajusta las rutas de `texto.txt` y `patrones.txt` en el código según tu estructura de directorios.

## Comparación de Rendimiento

Cada ejercicio ejecuta primero una versión secuencial (solo en el proceso 0) y luego la versión MPI, permitiendo comparar:
- Tiempo de ejecución
- Speedup obtenido
- Eficiencia de la paralelización
