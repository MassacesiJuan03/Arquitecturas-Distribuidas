# Script de PowerShell para ejecutar ejercicios MPI en WSL

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "  TP3 - Ejercicios con MPI" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Verificar si WSL está instalado
$wslInstalled = wsl --status 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: WSL no está instalado" -ForegroundColor Red
    Write-Host "Instala WSL con: wsl --install" -ForegroundColor Yellow
    exit 1
}

Write-Host "Compilando ejercicios en WSL..." -ForegroundColor Green
Write-Host ""

# Compilar todos los ejercicios
wsl bash -c "cd code && mpic++ -o ej1_mpi ej1.cpp -std=c++11 && echo '✓ Ejercicio 1 compilado'"
wsl bash -c "cd code && mpic++ -o ej2_mpi ej2.cpp -std=c++11 && echo '✓ Ejercicio 2 compilado'"
wsl bash -c "cd code && mpic++ -o ej3_mpi ej3.cpp -std=c++11 && echo '✓ Ejercicio 3 compilado'"
wsl bash -c "cd code && mpic++ -o ej4_mpi ej4.cpp -std=c++11 && echo '✓ Ejercicio 4 compilado'"

Write-Host ""
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Menú de opciones
Write-Host "Selecciona el ejercicio a ejecutar:" -ForegroundColor Yellow
Write-Host "1. Ejercicio 1 - Logaritmo con Serie de Taylor"
Write-Host "2. Ejercicio 2 - Rabin-Karp"
Write-Host "3. Ejercicio 3 - Multiplicación de Matrices"
Write-Host "4. Ejercicio 4 - Números Primos"
Write-Host "5. Ejecutar todos"
Write-Host "0. Salir"
Write-Host ""

$opcion = Read-Host "Opción"

switch ($opcion) {
    "1" {
        Write-Host "`nEjecutando Ejercicio 1 con 4 procesos..." -ForegroundColor Green
        wsl bash -c "cd code && mpirun -np 4 ./ej1_mpi"
    }
    "2" {
        Write-Host "`nEjecutando Ejercicio 2 con 4 procesos..." -ForegroundColor Green
        Write-Host "Nota: Requiere texto.txt y patrones.txt" -ForegroundColor Yellow
        wsl bash -c "cd code && mpirun -np 4 ./ej2_mpi"
    }
    "3" {
        Write-Host "`nEjecutando Ejercicio 3 con 4 procesos..." -ForegroundColor Green
        $size = Read-Host "Ingresa el tamaño de la matriz (ej: 500)"
        wsl bash -c "cd code && echo $size | mpirun -np 4 ./ej3_mpi"
    }
    "4" {
        Write-Host "`nEjecutando Ejercicio 4 con 4 procesos..." -ForegroundColor Green
        $n = Read-Host "Ingresa N (ej: 10000)"
        wsl bash -c "cd code && echo $n | mpirun -np 4 ./ej4_mpi"
    }
    "5" {
        Write-Host "`n=== EJERCICIO 1 ===" -ForegroundColor Green
        wsl bash -c "cd code && mpirun -np 4 ./ej1_mpi"
        
        Write-Host "`n=== EJERCICIO 3 ===" -ForegroundColor Green
        wsl bash -c "cd code && echo 500 | mpirun -np 4 ./ej3_mpi"
        
        Write-Host "`n=== EJERCICIO 4 ===" -ForegroundColor Green
        wsl bash -c "cd code && echo 10000 | mpirun -np 4 ./ej4_mpi"
    }
    "0" {
        Write-Host "Saliendo..." -ForegroundColor Yellow
    }
    default {
        Write-Host "Opción no válida" -ForegroundColor Red
    }
}

Write-Host "`n¡Listo!" -ForegroundColor Green
