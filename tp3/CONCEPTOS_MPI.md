# Conceptos MPI Implementados en el TP3

## Introducción a MPI

**MPI (Message Passing Interface)** es un estándar para comunicación en sistemas de memoria distribuida. A diferencia de hilos (threads) que comparten memoria, los procesos MPI se comunican mediante mensajes explícitos.

## 1. Estructura Básica de un Programa MPI

### 1.1 Inicialización y Finalización

```cpp
int main(int argc, char** argv) {
    // Inicializar MPI - SIEMPRE es lo primero
    MPI_Init(&argc, &argv);
    
    // ... código del programa ...
    
    // Finalizar MPI - SIEMPRE es lo último
    MPI_Finalize();
    return 0;
}
```

**¿Por qué es importante?**
- `MPI_Init()` prepara el entorno MPI y debe llamarse antes de cualquier función MPI
- `MPI_Finalize()` limpia los recursos y debe ser lo último antes de salir del programa
- Todo código MPI debe estar entre estas dos llamadas

### 1.2 Identificación de Procesos

```cpp
int world_size;  // Número total de procesos
int rank;        // ID único de este proceso (0 a world_size-1)

// Obtener el número total de procesos
MPI_Comm_size(MPI_COMM_WORLD, &world_size);

// Obtener el ID (rank) de este proceso
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

**Concepto clave:**
- Cada proceso ejecuta el mismo código pero con diferente `rank`
- El `rank` se usa para que cada proceso haga tareas diferentes
- El proceso con `rank = 0` suele ser el "maestro" o coordinador

## 2. Comunicaciones Colectivas

Las comunicaciones colectivas involucran a todos los procesos del comunicador.

### 2.1 MPI_Bcast - Broadcast (Difusión)

```cpp
// El proceso 0 envía datos a TODOS los demás
MPI_Bcast(&dato, 1, MPI_INT, 0, MPI_COMM_WORLD);
```

**¿Cómo funciona?**
```
Proceso 0:  [dato = 100] ---> broadcast
                                ↓
Proceso 1:  [ ] ← recibe      [dato = 100]
Proceso 2:  [ ] ← recibe      [dato = 100]
Proceso 3:  [ ] ← recibe      [dato = 100]
```

**Uso en nuestros ejercicios:**
- **Ej1**: Broadcast del valor de `x` a todos los procesos
- **Ej2**: Broadcast del texto completo a todos los procesos
- **Ej3**: Broadcast de las matrices A y B
- **Ej4**: Broadcast del valor de `N`

### 2.2 MPI_Reduce - Reducción

```cpp
// Combinar valores de todos los procesos usando una operación
MPI_Reduce(&dato_local, &resultado, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```

**¿Cómo funciona?**
```
Proceso 0:  [5] ──┐
Proceso 1:  [3] ──┤
Proceso 2:  [7] ──┼─→ MPI_SUM ──→ [22] (en proceso 0)
Proceso 3:  [7] ──┘
```

**Operaciones disponibles:**
- `MPI_SUM`: Suma todos los valores
- `MPI_MAX`: Encuentra el máximo
- `MPI_MIN`: Encuentra el mínimo
- `MPI_PROD`: Multiplica todos los valores

**Uso en Ejercicio 1:**
```cpp
// Cada proceso calcula su parte de la serie
double resultado_local = log_taylor_mpi(x, inicio, fin);

// Sumar todos los resultados parciales
MPI_Reduce(&resultado_local, &resultado_total, 1, MPI_DOUBLE, 
           MPI_SUM, 0, MPI_COMM_WORLD);
```

### 2.3 MPI_Gather y MPI_Gatherv - Recolección

```cpp
// Recolectar datos de tamaño fijo de todos los procesos
MPI_Gather(&dato_local, count, MPI_INT, 
           &datos_totales, count, MPI_INT, 
           0, MPI_COMM_WORLD);

// Recolectar datos de tamaño variable
MPI_Gatherv(&datos_locales[0], local_count, MPI_FLOAT,
            &datos_totales[0], recvcounts, displs, MPI_FLOAT,
            0, MPI_COMM_WORLD);
```

**¿Cómo funciona MPI_Gather?**
```
Proceso 0:  [1, 2] ──┐
Proceso 1:  [3, 4] ──┼─→ Gather ─→ [1, 2, 3, 4, 5, 6] (en proceso 0)
Proceso 2:  [5, 6] ──┘
```

**Uso en Ejercicio 3:**
- Cada proceso calcula algunas filas de la matriz resultado
- `MPI_Gatherv` recolecta todas las filas en el proceso 0
- Se usa `Gatherv` (con v) porque el último proceso puede tener más filas

### 2.4 MPI_Barrier - Sincronización

```cpp
// Esperar a que todos los procesos lleguen a este punto
MPI_Barrier(MPI_COMM_WORLD);
```

**¿Para qué sirve?**
```
Proceso 0:  ━━━━━━━━━━━━━━|  ← llega primero, espera
Proceso 1:  ━━━━━━━━━━━|     ← llega después
Proceso 2:  ━━━━━━━━━━━━━━━━|  ← último en llegar
            TODOS CONTINÚAN JUNTOS →
```

**Uso:** Sincronizar antes de medir tiempos para resultados precisos

## 3. Comunicaciones Punto a Punto

Comunicación directa entre dos procesos específicos.

### 3.1 MPI_Send y MPI_Recv

```cpp
// Proceso A envía
MPI_Send(&dato, 1, MPI_INT, destino, tag, MPI_COMM_WORLD);

// Proceso B recibe
MPI_Recv(&dato, 1, MPI_INT, origen, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

**Parámetros:**
- `&dato`: puntero a los datos
- `1`: número de elementos
- `MPI_INT`: tipo de dato
- `destino/origen`: rank del otro proceso
- `tag`: etiqueta para identificar el mensaje
- `MPI_COMM_WORLD`: comunicador
- `MPI_STATUS_IGNORE`: ignorar información del estado

**Uso en Ejercicio 2:**
```cpp
if (rank == 0) {
    // Proceso 0 envía patrones a otros procesos
    for (int dest = 1; dest < world_size; dest++) {
        MPI_Send(&pattern_size, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
        MPI_Send(&pattern[0], pattern_size, MPI_CHAR, dest, 1, MPI_COMM_WORLD);
    }
} else {
    // Otros procesos reciben sus patrones
    MPI_Recv(&pattern_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&pattern[0], pattern_size, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
```

**Uso en Ejercicio 4:**
```cpp
if (rank == 0) {
    // Recibir primos de otros procesos
    MPI_Recv(&count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&primos[0], count, MPI_LONG_LONG, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
} else {
    // Enviar primos al proceso 0
    MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&primos[0], count, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
}
```

## 4. Tipos de Datos MPI

Los tipos de datos de C/C++ tienen equivalentes en MPI:

| C/C++ Type          | MPI Type           |
|---------------------|-------------------|
| `char`              | `MPI_CHAR`        |
| `int`               | `MPI_INT`         |
| `long long`         | `MPI_LONG_LONG`   |
| `float`             | `MPI_FLOAT`       |
| `double`            | `MPI_DOUBLE`      |
| `unsigned long long`| `MPI_UNSIGNED_LONG_LONG` |

## 5. Medición de Tiempo en MPI

```cpp
double tiempo_inicio = MPI_Wtime();
// ... código a medir ...
double tiempo_fin = MPI_Wtime();
double tiempo_total = tiempo_fin - tiempo_inicio;
```

**Ventajas sobre chrono:**
- Sincronizado entre procesos
- Alta precisión
- Específico para aplicaciones paralelas

## 6. Patrones de Paralelización

### 6.1 División de Datos (Data Decomposition)

**Usado en: Ejercicio 1, 3, 4**

```cpp
// Cada proceso trabaja con una porción de los datos
long long bloque = N / world_size;
long long inicio = rank * bloque;
long long fin = (rank == world_size - 1) ? N : (rank + 1) * bloque;

// Procesar mi rango
for (long long i = inicio; i < fin; i++) {
    // procesar elemento i
}
```

### 6.2 División de Tareas (Task Decomposition)

**Usado en: Ejercicio 2**

```cpp
// Cada proceso realiza tareas diferentes
// Proceso 0: busca patrones 0-9
// Proceso 1: busca patrones 10-19
// Proceso 2: busca patrones 20-29
// etc.
```

### 6.3 Patrón Maestro-Trabajador

**Usado en: Todos los ejercicios**

```cpp
if (rank == 0) {
    // Proceso maestro:
    // - Lee datos de entrada
    // - Distribuye trabajo
    // - Recolecta resultados
    // - Muestra output
} else {
    // Procesos trabajadores:
    // - Reciben su tarea
    // - Procesan
    // - Envían resultados
}
```

## 7. Buenas Prácticas

### 7.1 Balanceo de Carga

```cpp
// MAL: último proceso puede tener mucho más trabajo
int bloque = N / world_size;
int inicio = rank * bloque;
int fin = (rank + 1) * bloque; // ¡El último proceso pierde los N % world_size elementos!

// BIEN: el último proceso toma el resto
if (rank == world_size - 1) {
    fin = N;  // Asegura que todos los elementos se procesan
}
```

### 7.2 Minimizar Comunicación

```cpp
// MAL: comunicar en cada iteración
for (int i = 0; i < N; i++) {
    resultado = calcular(i);
    MPI_Send(&resultado, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
}

// BIEN: calcular todo localmente, comunicar una vez
vector<double> resultados;
for (int i = 0; i < N; i++) {
    resultados.push_back(calcular(i));
}
MPI_Send(&resultados[0], N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
```

### 7.3 Evitar Deadlocks

```cpp
// ¡CUIDADO! Esto causa deadlock si todos esperan recibir primero
if (rank == 0) {
    MPI_Recv(...);  // Espera recibir
    MPI_Send(...);  // Nunca llega aquí
}
if (rank == 1) {
    MPI_Recv(...);  // Espera recibir - ¡DEADLOCK!
    MPI_Send(...);
}

// CORRECTO: alternar el orden
if (rank == 0) {
    MPI_Send(...);  // Envía primero
    MPI_Recv(...);  // Luego recibe
}
if (rank == 1) {
    MPI_Recv(...);  // Recibe primero
    MPI_Send(...);  // Luego envía
}
```

## 8. Análisis de Rendimiento

### 8.1 Speedup

```
Speedup = T_secuencial / T_paralelo
```

- **Speedup ideal**: igual al número de procesos (N procesos → speedup de N)
- **Speedup real**: generalmente menor por overhead de comunicación

### 8.2 Eficiencia

```
Eficiencia = Speedup / Número_de_procesos
```

- **Eficiencia ideal**: 1.0 (100%)
- **Eficiencia real**: típicamente 0.7 - 0.9 (70-90%)

### 8.3 Factores que Afectan el Rendimiento

1. **Overhead de comunicación**: Tiempo enviando/recibiendo mensajes
2. **Desbalanceo de carga**: Algunos procesos terminan antes
3. **Sincronización**: Tiempo esperando en barreras
4. **Latencia de red**: En sistemas distribuidos

## 9. Comparación: MPI vs Threads

| Aspecto            | MPI                      | Threads              |
|--------------------|--------------------------|----------------------|
| Memoria            | Distribuida (mensajes)   | Compartida           |
| Escalabilidad      | Alta (múltiples nodos)   | Limitada (1 nodo)    |
| Comunicación       | Explícita (Send/Recv)    | Implícita (variables)|
| Sincronización     | Barreras, mensajes       | Mutex, condiciones   |
| Complejidad        | Mayor                    | Menor                |
| Debugging          | Más difícil              | Más fácil            |

## 10. Ejercicios del TP3 - Resumen

### Ejercicio 1: Logaritmo (Serie de Taylor)
- **Patrón**: División de datos
- **Comunicación**: Broadcast (entrada), Reduce (resultado)
- **Complejidad**: Simple, buen speedup esperado

### Ejercicio 2: Rabin-Karp
- **Patrón**: División de tareas
- **Comunicación**: Broadcast (texto), Send/Recv (patrones y resultados)
- **Complejidad**: Media, comunicación intensiva

### Ejercicio 3: Multiplicación de Matrices
- **Patrón**: División por filas
- **Comunicación**: Broadcast (matrices), Gatherv (resultado)
- **Complejidad**: Media-Alta, buen paralelismo

### Ejercicio 4: Números Primos
- **Patrón**: División de rango
- **Comunicación**: Broadcast (N), Send/Recv (primos)
- **Complejidad**: Alta, requiere generación de primos base

## Conclusión

MPI es una herramienta poderosa para computación paralela en sistemas distribuidos. Los conceptos clave son:

1. **Procesos independientes** con memoria propia
2. **Comunicación explícita** mediante mensajes
3. **Escalabilidad** a múltiples nodos
4. **Patrones de paralelización** bien definidos
5. **Trade-off** entre comunicación y computación
