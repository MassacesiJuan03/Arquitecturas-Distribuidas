#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <cmath>
#include <chrono>
#include <sys/time.h>

using namespace std;
using namespace std::chrono;

long long N = 10000000;

double log_taylor_whithout_threads(double x)
{
    double r = (x - 1) / (x + 1);
    double sum = 0.0;
    double pot = r; // r^n, comenzamos en n=0
    for (long long n = 0; n < N; n++)
    {
        sum += pot / (2 * n + 1);
        pot *= r * r; // r^(2n+2) para el siguiente término
    }

    return 2 * sum; // ln(x) = 2 * sum
    
}

void log_taylor_multithreaded(double x, long long ini, long long fin, long double &resultado)
{
    double r = (x - 1) / (x + 1);
    double sum = 0.0;
    double pot = pow(r, ini); // arrancamos en r^ini

    for (long long n = ini; n <= fin; n++)
    {
        sum += pot / (2 * n + 1);
        pot *= r * r; // r^(2n+2) para el siguiente término
    }
    resultado = 2 * sum; // ln(x) = 2 * sum
}

int main()
{
    long double x = 1600000; // Valor de x
    int hilos = 4;

    //cout << "Ingrese el valor de x (> 1500000): ";
    //cin >> x;
    //cout << "Ingrese el numero de hilos (divisor de 10000000): ";
    //cin >> hilos;

    if (x <= 1500000 || 10000000 % hilos != 0)
    {
        cout << "Valores invalidos. Asegurese que x > 1500000 y que el numero de hilos sea un divisor de 10000000." << endl;
        return 0;
    }

    // ---------------------- SECUENCIAL ----------------------
    auto t1 = high_resolution_clock::now();
    timeval time1,time2;
    gettimeofday(&time1,NULL);

    long double resultado_secuencial = log_taylor_whithout_threads(x);

    gettimeofday(&time2,NULL);
    cout << fixed;
    cout.precision(10);

    cout << "\n[SECUENCIAL] ln(" << x << ") ≈ " << resultado_secuencial << endl;
    cout << "Tiempo ejecución: " << (double(time2.tv_sec - time1.tv_sec) + double(time2.tv_usec-time1.tv_usec)/1000000) * 1000.0 << " ms" << endl;
    auto t2 = high_resolution_clock::now();
    auto duracion_seq = duration_cast<milliseconds>(t2 - t1).count();
    
    cout << "Tiempo secuencial: " << duracion_seq << " ms" << endl;

    // ---------------------- PARALELO ----------------------
    auto t3 = high_resolution_clock::now();

    vector<thread> workers;
    vector<long double> resultados(hilos, 0.0);

    long long bloque = N / hilos;
    for (int i = 0; i < hilos; i++)
    {
        if (i == 0) {
            workers.push_back(thread(log_taylor_multithreaded, x, 0, bloque - 1, ref(resultados[i])));
        }
        else if (i == hilos - 1)
        {
            workers.push_back(thread(log_taylor_multithreaded, x, i * bloque, N - 1, ref(resultados[i])));
        }
        else
        {
            workers.push_back(thread(log_taylor_multithreaded, x, i * bloque, (i + 1) * bloque - 1, ref(resultados[i])));
        }
    }

    for (auto &th : workers) th.join();

    auto t4 = high_resolution_clock::now();
    auto duracion_par = duration_cast<milliseconds>(t4 - t3).count();

    cout << "Tiempo paralelo: " << duracion_par << " ms" << endl;

    //Speedup
    cout << "Speedup: " << double(duracion_seq) / double(duracion_par) << endl;

    return 0;
}