#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

// Rabin-Karp con rolling hash: devuelve la cantidad de ocurrencias de un patrón
size_t RabinKarp(const string& text, const string& pattern) {
    int n = text.size(), m = pattern.size();
    if (m == 0 || m > n) return 0;

    const unsigned long long d = 256;
    const unsigned long long q = 1000000007;

    unsigned long long h = 1, hashPattern = 0, hashText = 0;
    size_t count = 0;

    for (int i = 0; i < m - 1; i++)
        h = (h * d) % q;

    for (int i = 0; i < m; i++) {
        hashPattern = (hashPattern * d + static_cast<unsigned char>(pattern[i])) % q;
        hashText    = (hashText    * d + static_cast<unsigned char>(text[i])) % q;
    }

    for (int i = 0; i <= n - m; i++) {
        if (hashText == hashPattern){
            bool match = true;
            for (int j = 0; j < m; j++) {
                if (text[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) count++;
        }
        if (i < n - m) {
            hashText = (d * (hashText - static_cast<unsigned char>(text[i]) * h) + static_cast<unsigned char>(text[i + m])) % q;
            if (hashText < 0) hashText += q;
        }
    }
    return count;
}

vector<size_t> RabinKarpSequential(const string& text, const vector<string>& patterns) {
    vector<size_t> counts;
    counts.reserve(patterns.size());
    for (const auto& p : patterns)
        counts.push_back(RabinKarp(text, p));
    return counts;
}

bool leer_texto(const string& ruta, string& texto) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo) return false;
    texto.assign((istreambuf_iterator<char>(archivo)), istreambuf_iterator<char>());
    return true;
}

bool leer_patrones(const string& ruta, vector<string>& patterns) {
     ifstream archivo(ruta);
    if (!archivo) return false;
    string linea;
    while (getline(archivo, linea)) if (!linea.empty()) patterns.push_back(linea);
    return true;
}

int main() {
    string texto;
    vector<string> patterns;

    string ruta_texto = "/home/juan-ignacio/Escritorio/Facultad/Arquitecturas-Distribuidas/tp1-Paralelismo a nivel de hilos/texto.txt";
    string ruta_patrones = "/home/juan-ignacio/Escritorio/Facultad/Arquitecturas-Distribuidas/tp1-Paralelismo a nivel de hilos/patrones.txt";

    if (!leer_texto(ruta_texto, texto)) {
        cerr << "No se pudo leer texto.txt\n";
        return 1;
    }
    if (!leer_patrones(ruta_patrones, patterns)) {
        cerr << "No se pudo leer patrones.txt\n";
        return 2;
    }

    auto counts = RabinKarpSequential(texto, patterns);

    for (size_t i = 0; i < counts.size(); i++)
        cout << "El patrón " << i << " aparece " << counts[i] << " veces\n";

    return 0;
}
