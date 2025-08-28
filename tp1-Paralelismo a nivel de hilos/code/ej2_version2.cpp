#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

// Función para contar cuántas veces aparece `pattern` en `text`
size_t count_occurrences(const string& text, const string& pattern) {
    size_t count = 0;
    size_t pos = text.find(pattern, 0);
    while (pos != string::npos) {
        count++;
        pos = text.find(pattern, pos + 1);
    }
    return count;
}

int main() {
    // Leer todo el texto
    ifstream text_file("/home/juan-ignacio/Escritorio/Facultad/Arquitecturas-Distribuidas/tp1-Paralelismo a nivel de hilos/texto.txt", ios::binary);
    if (!text_file) {
        cerr << "No se pudo abrir texto.txt\n";
        return 1;
    }
    string text((istreambuf_iterator<char>(text_file)), istreambuf_iterator<char>());
    text_file.close();

    // Leer patrones
    ifstream pattern_file("/home/juan-ignacio/Escritorio/Facultad/Arquitecturas-Distribuidas/tp1-Paralelismo a nivel de hilos/patrones.txt");
    if (!pattern_file) {
        cerr << "No se pudo abrir patrones.txt\n";
        return 1;
    }
    vector<string> patterns;
    string line;
    while (getline(pattern_file, line)) {
        patterns.push_back(line);
    }
    pattern_file.close();

    // Medir tiempo de ejecución
    auto start = high_resolution_clock::now();

    // Contar apariciones
    vector<size_t> counts(patterns.size(), 0);
    for (size_t i = 0; i < patterns.size(); ++i) {
        counts[i] = count_occurrences(text, patterns[i]);
    }

    auto end = high_resolution_clock::now();
    double elapsed_sec = duration<double>(end - start).count();

    // Mostrar resultados
    cout << "Resultados secuenciales:\n";
    for (size_t i = 0; i < counts.size(); ++i) {
        cout << "El patron " << i << " aparece " << counts[i] << " veces\n";
    }

    cout << "Tiempo de ejecución secuencial: " << elapsed_sec << " segundos\n";
    cout << "----------------------------------------\n" << endl;

    // Paralelo
    vector<thread> workers;
    vector<size_t> parallel_counts(patterns.size(), 0);

    start = high_resolution_clock::now();
    for (size_t i = 0; i < patterns.size(); ++i) {
        workers.emplace_back([&, i]() {
            parallel_counts[i] = count_occurrences(text, patterns[i]);
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    end = high_resolution_clock::now();
    double elapsed_par = duration<double>(end - start).count();

    // Mostrar resultados paralelos
    cout << "Resultados paralelos:\n";

    for (size_t i = 0; i < parallel_counts.size(); ++i) {
        cout << "El patron " << i << " aparece " << parallel_counts[i] << " veces\n";
    }
    cout << "Tiempo de ejecución paralelo: " << elapsed_par << " segundos\n";

    return 0;

    //Speedup
    cout << "Speedup: " << elapsed_sec / elapsed_par << endl;
}
