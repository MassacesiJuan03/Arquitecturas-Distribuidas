#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;

mutex mtx;

// --------------------
// Chequear primalidad usando primos_base
// --------------------
bool esPrimo(long long n, const vector<long long>& primos_base) {
    if (n < 2) return false;
    if (n % 2 == 0 && n != 2) return false;
    for (auto p : primos_base) {
        if (p * p > n) break;      // condición de corte
        if (n % p == 0) return false;
    }
    return true;
}

// --------------------
// Paso 1: generar primos hasta sqrt(N)
// --------------------
vector<long long> generarPrimosBase(long long limite) {
    vector<long long> primos;
    for (long long i = 2; i <= limite; i++) {
        bool primo = true;
        for (long long j = 2; j * j <= i; j++) {
            if (i % j == 0) { primo = false; break; }
        }
        if (primo) primos.push_back(i);
    }
    return primos;
}

// --------------------
// SECUENCIAL (pasos 1-3)
// --------------------
vector<long long> primosSecuencial(long long N) {
    vector<long long> primos_base = generarPrimosBase(sqrt(N));
    vector<long long> primos;

    for (long long i = 2; i <= N; i++) {
        if (esPrimo(i, primos_base)) primos.push_back(i);
    }
    return primos;
}

// --------------------
// PARALELO (pasos 1-3)
// --------------------
void primosParcial(long long ini, long long fin, const vector<long long>& primos_base,
                   vector<long long>& resultado) {
    vector<long long> local;
    for (long long i = ini; i <= fin; i++) {
        if (i % 2 == 0 && i != 2) continue;  // descartar pares
        if (esPrimo(i, primos_base)) local.push_back(i);
    }
    lock_guard<mutex> lock(mtx);
    resultado.insert(resultado.end(), local.begin(), local.end());
}

vector<long long> primosParalelo(long long N, int numHilos) {
    vector<long long> primos_base = generarPrimosBase(sqrt(N));
    vector<long long> resultado;
    vector<thread> hilos;

    long long bloque = N / numHilos;
    for (int t = 0; t < numHilos; t++) {
        long long ini = t * bloque + (t == 0 ? 2 : 1); // arranca en 2
        long long fin = (t == numHilos - 1 ? N : (t + 1) * bloque);
        hilos.emplace_back(primosParcial, ini, fin, cref(primos_base), ref(resultado));
    }
    for (auto& th : hilos) th.join();

    sort(resultado.begin(), resultado.end()); // paso 3
    return resultado;
}

// --------------------
// MAIN
// --------------------
int main() {
    long long N;
    int numHilos = 8;
    cout << "Ingrese N: ";
    cin >> N;

    // ---- Secuencial ----
    auto t1 = chrono::high_resolution_clock::now();
    auto seq = primosSecuencial(N);
    auto t2 = chrono::high_resolution_clock::now();
    double tiempoSeq = chrono::duration<double>(t2 - t1).count();

    cout << "\n[Secuencial] " << seq.size() << " primos. Tiempo: "
         << tiempoSeq << " s\n";
    cout << "Ultimos 10 primos: ";
    for (int i = max(0, (int)seq.size() - 10); i < seq.size(); i++)
        cout << seq[i] << " ";
    cout << "\n";

    // ---- Paralelo ----
    auto t3 = chrono::high_resolution_clock::now();
    auto par = primosParalelo(N, numHilos);
    auto t4 = chrono::high_resolution_clock::now();
    double tiempoPar = chrono::duration<double>(t4 - t3).count();

    cout << "\n[Paralelo] " << par.size() << " primos. Tiempo: "
         << tiempoPar << " s\n";
    cout << "Ultimos 10 primos: ";
    for (int i = max(0, (int)par.size() - 10); i < par.size(); i++)
        cout << par[i] << " ";
    cout << "\n";

    cout << "\nSpeedup = " << tiempoSeq / tiempoPar << "\n";

    return 0;
}
