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
#include "tsp.h"

struct Config {
    int useGenerator = 0; // 0 - z pliku, 1 - losowa ATSP, 2 - losowa STSP
    std::string inputFile;
    std::string outputFile;
    int repetitions = 1;
    int instanceSize = 0;
    int randIterations = 1000; // Ilość iteracji dla algorytmu RAND
    bool showProgress = false;
};

Config readConfig(const std::string& filename) {
    Config cfg;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Blad: Nie mozna otworzyc pliku konfiguracyjnego: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '\r') continue;

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                if (!value.empty() && value.back() == '\r') {
                    value.pop_back();
                }

                if (key == "use_generator") cfg.useGenerator = std::stoi(value);
                else if (key == "input_file") cfg.inputFile = value;
                else if (key == "output_file") cfg.outputFile = value;
                else if (key == "repetitions") cfg.repetitions = std::stoi(value);
                else if (key == "instance_size") cfg.instanceSize = std::stoi(value);
                else if (key == "rand_iterations") cfg.randIterations = std::stoi(value);
                else if (key == "show_progress") cfg.showProgress = (value == "1");
            }
        }
    }
    return cfg;
}

std::vector<std::vector<int>> loadMatrix(const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Blad: Nie mozna otworzyc pliku z macierza: " << filename << std::endl;
        exit(1);
    }

    int size;
    file >> size; 

    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file >> matrix[i][j];
        }
    }

    file.close();
    return matrix;
}

// gemerator asymetrycznej macierzy ATSP
std::vector<std::vector<int>> generateRandomATSP(int size) {
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (i == j) matrix[i][j] = -1;
            else matrix[i][j] = dist(gen);
        }
    }
    return matrix;
}

// generator symetrycznych macierzy STSP - taki sam koszt w obie strony
std::vector<std::vector<int>> generateRandomSTSP(int size) {
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    for (int i = 0; i < size; ++i) {
        for (int j = i; j < size; ++j) {
            if (i == j) {
                matrix[i][j] = -1;
            } else {
                int cost = dist(gen);
                matrix[i][j] = cost;
                matrix[j][i] = cost; // symetryczna macierz - ten sam koszt w obie strony
            }
        }
    }
    return matrix;
}

// funkcja pomocnicza do wyświetlania macierzy (zakomentowac przed oddaniem, sluzy tylko dla testpowania generatora)
void printMatrix(const std::vector<std::vector<int>>& matrix) {
    std::cout << "Wygenerowana macierz:\n";
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            std::cout << std::setw(4) << matrix[i][j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

SIZE_T getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024; // dzielimy przez 1024, aby uzyskać wynik w KB
    }
    return 0;
}

double relativeError(int approx, int optimum) {
    if (optimum <= 0) return 0.0;
    return 100.0 * (double)(approx - optimum) / (double)optimum;
}

int main() {
    std::string configPath = "config.txt"; 
    Config cfg = readConfig(configPath);

    std::vector<std::vector<int>> matrix;

    std::cout << "========================================\n";

    if (cfg.useGenerator == 1) {
        std::cout << "Tryb: Generowanie losowej macierzy ATSP.\n";
        std::cout << "Rozmiar instancji: " << cfg.instanceSize << "x" << cfg.instanceSize << "\n";
        matrix = generateRandomATSP(cfg.instanceSize);

    } else if (cfg.useGenerator == 2) {
        std::cout << "Tryb: Generowanie losowej macierzy STSP.\n";
        std::cout << "Rozmiar instancji: " << cfg.instanceSize << "x" << cfg.instanceSize << "\n";
        matrix = generateRandomSTSP(cfg.instanceSize);

    } else {
        std::cout << "Tryb: Wczytywanie macierzy z pliku.\n";
        std::cout << "Plik wejsciowy: " << cfg.inputFile << "\n";
        matrix = loadMatrix(cfg.inputFile);
        std::cout << "Rozmiar instancji: " << matrix.size() << "x" << matrix.size() << "\n";
    }
    std::cout << "Powtorzenia: " << cfg.repetitions << "\n";
    std::cout << "========================================\n\n";

    TSP tsp(matrix);
    std::vector<int> best_path;
    int best_cost;

    std::ofstream outFile(cfg.outputFile, std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Blad: Nie mozna otworzyc pliku do zapisu CSV.\n";
        return 1;
    }

    std::cout << "--- Rozpoczynamy testy algorytmow ---\n";

    // 1 -brute force
    double total_time_bf = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "Brute force postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.bruteForce(best_path);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_bf += duration.count();
        outFile << "BruteForce," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "Brute Force: Koszt = " << best_cost << ", Sredni czas = " << total_time_bf / cfg.repetitions << " ms\n\n";

    // 2 - nearest neighbor
    double total_time_nn = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "Nearest neighbor postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.nearestNeighbor(best_path, 0); 
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_nn += duration.count();
        outFile << "NearestNeighbor," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "Nearest Neighbor: Koszt = " << best_cost << ", Sredni czas = " << total_time_nn / cfg.repetitions << " ms\n\n";

    // 3 - rnn
    double total_time_rnn = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "RNN postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.repetitiveNearestNeighbor(best_path); 
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_rnn += duration.count();
        outFile << "RNN," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "RNN: Koszt = " << best_cost << ", Sredni czas = " << total_time_rnn / cfg.repetitions << " ms\n\n";

    // 4 - random walk
    double total_time_rnd = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "Random postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.randomWalk(best_path, cfg.randIterations); // przekazujemy ilosc iteracji z configu
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_rnd += duration.count();
        outFile << "Random," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "RAND (iteracji: " << cfg.randIterations << "): Koszt (z ostatniej proby) = " << best_cost << ", Sredni czas = " << total_time_rnd / cfg.repetitions << " ms\n";

    outFile.close();
    std::cout << "\nTesty zakonczone. Wyniki w: " << cfg.outputFile << "\n";

    std::cout << "========================================\n";
    std::cout << "Zajeta pamiec (RAM): " << getMemoryUsage() << " KB\n";
    std::cout << "========================================\n";

    return 0;
}