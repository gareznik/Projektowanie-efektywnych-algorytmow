#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>
#include <windows.h>
#include <psapi.h>

#include "tsplib_parser.h"
#include "bnb.h"

using namespace std;

struct Config {
    int useGenerator = 1; // 0 - z pliku, 1 - losowa ATSP, 2 - losowa STSP
    string inputFile;
    string outputFile;
    int repetitions = 1;
    int instanceSize = 0;
    int timeLimitS = 300;
    bool showProgress = false;
};

Config readConfig(const string& filename) {
    Config cfg;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Blad: Nie mozna otworzyc pliku konfiguracyjnego: " << filename << endl;
        exit(1);
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '\r') continue;

        istringstream is_line(line);
        string key;
        if (getline(is_line, key, '=')) {
            string value;
            if (getline(is_line, value)) {
                if (!value.empty() && value.back() == '\r') {
                    value.pop_back();
                }

                if (key == "use_generator") cfg.useGenerator = stoi(value);
                else if (key == "input_file") cfg.inputFile = value;
                else if (key == "output_file") cfg.outputFile = value;
                else if (key == "repetitions") cfg.repetitions = stoi(value);
                else if (key == "instance_size") cfg.instanceSize = stoi(value);
                else if (key == "time_limit_s") cfg.timeLimitS = stoi(value);
                else if (key == "show_progress") cfg.showProgress = (value == "1");
            }
        }
    }
    return cfg;
}

vector<vector<int>> generateRandomATSP(int size) {
    vector<vector<int>> matrix(size, vector<int>(size));
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 100);

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (i == j) matrix[i][j] = -1;
            else matrix[i][j] = dist(gen);
        }
    }
    return matrix;
}

vector<vector<int>> generateRandomSTSP(int size) {
    vector<vector<int>> matrix(size, vector<int>(size));
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 100);

    for (int i = 0; i < size; ++i) {
        for (int j = i; j < size; ++j) {
            if (i == j) {
                matrix[i][j] = -1;
            } else {
                int cost = dist(gen);
                matrix[i][j] = cost;
                matrix[j][i] = cost; // symetryczna macierz
            }
        }
    }
    return matrix;
}

SIZE_T getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024; // wynik w KB
    }
    return 0;
}

int main() {
    // Przypięcie do 1 rdzenia
    SetProcessAffinityMask(GetCurrentProcess(), 1);

    string configPath = "config.txt"; 
    Config cfg = readConfig(configPath);

    vector<vector<int>> matrix;

    cout << "========================================\n";

    if (cfg.useGenerator == 1) {
        cout << "Tryb: Generowanie losowej macierzy ATSP.\n";
        cout << "Rozmiar instancji: " << cfg.instanceSize << "x" << cfg.instanceSize << "\n";
        matrix = generateRandomATSP(cfg.instanceSize);

    } else if (cfg.useGenerator == 2) {
        cout << "Tryb: Generowanie losowej macierzy STSP.\n";
        cout << "Rozmiar instancji: " << cfg.instanceSize << "x" << cfg.instanceSize << "\n";
        matrix = generateRandomSTSP(cfg.instanceSize);

    } else {
        cout << "Tryb: Wczytywanie macierzy z pliku.\n";
        cout << "Plik wejsciowy: " << cfg.inputFile << "\n";
        if (cfg.inputFile.empty()) {
            cerr << "Blad: Sciezka do pliku (input_file) jest pusta!\n";
            return 1;
        }
        matrix = TSPLibParser::loadMatrix(cfg.inputFile);
        if (!matrix.empty()) {
            cout << "Rozmiar instancji: " << matrix.size() << "x" << matrix.size() << "\n";
        }
    }

    if (matrix.empty() || matrix.size() < 3) {
        cerr << "Blad: Macierz jest pusta lub za mala!\n";
        return 1;
    }

    cout << "Powtorzenia: " << cfg.repetitions << "\n";
    cout << "========================================\n\n";

    ofstream outFile(cfg.outputFile, ios::app);
    if (!outFile.is_open()) {
        cerr << "Blad: Nie mozna otworzyc pliku do zapisu CSV.\n";
        return 1;
    }

    cout << "--- Rozpoczynamy testy algorytmow ---\n\n";

    // UWAGA: Przekazujemy 'false' jako trzeci parametr, aby bnb.h nie spamił w konsoli, 
    // robimy pasek postępu tutaj, tak jak w projekt 1.
    BranchAndBound bnb(matrix, cfg.timeLimitS, false);
    string algos[] = {"", "BFS", "DFS", "LowestCost"};

    for (int s = 1; s <= 3; ++s) {
        double total_time = 0;
        int best_cost = 0;
        bool timeout = false;

        for (int i = 0; i < cfg.repetitions; ++i) {
            if (cfg.showProgress) {
                cout << algos[s] << " postep: " << i + 1 << "/" << cfg.repetitions << "\r" << flush;
            }
            
            auto start = chrono::high_resolution_clock::now();
            int cost = bnb.solve(s);
            auto end = chrono::high_resolution_clock::now();
            
            chrono::duration<double, milli> duration = end - start;

            if (cost == -1) {
                timeout = true;
                outFile << algos[s] << "," << matrix.size() << "," << i + 1 << ",TIMEOUT," << duration.count() << "\n";
                if (cfg.showProgress) cout << "\n";
                cout << algos[s] << " -> TIMEOUT (> " << cfg.timeLimitS << "s)\n\n";
                break; 
            }

            best_cost = cost;
            total_time += duration.count();
            
            // Zapis w CSV (z przecinkami, jak w pierwszym projekcie)
            outFile << algos[s] << "," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
        }

        if (!timeout) {
            if (cfg.showProgress) cout << "\n";
            cout << algos[s] << ": Koszt = " << best_cost << ", Sredni czas = " << total_time / cfg.repetitions << " ms\n\n";
        }
    }

    outFile.close();
    cout << "Testy zakonczone. Wyniki w: " << cfg.outputFile << "\n\n";

    cout << "========================================\n";
    cout << "Zajeta pamiec (RAM): " << getMemoryUsage() << " KB\n";
    cout << "========================================\n\n";

    return 0;
}