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
    bool useGenerator = false;
    std::string inputFile;
    std::string outputFile;
    int repetitions = 1;
    int instanceSize = 0;
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

                if (key == "use_generator") cfg.useGenerator = (value == "1");
                else if (key == "input_file") cfg.inputFile = value;
                else if (key == "output_file") cfg.outputFile = value;
                else if (key == "repetitions") cfg.repetitions = std::stoi(value);
                else if (key == "instance_size") cfg.instanceSize = std::stoi(value);
                else if (key == "show_progress") cfg.showProgress = (value == "1");
            }
        }
    }
    return cfg;
}

// generator matryc losowych atsp
std::vector<std::vector<int>> generateRandomMatrix(int size) {
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100); // odlełości od 1 do 100

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (i == j) {
                matrix[i][j] = -1; // zawsze -1 po przekątnej
            } else {
                matrix[i][j] = dist(gen);
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
        return pmc.WorkingSetSize / 1024; // Делим на 1024, чтобы получить Килобайты
    }
    return 0;
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

int main() {
    std::string configPath = "config.txt"; 
    Config cfg = readConfig(configPath);

    std::cout << "--- Wczytana konfiguracja ---\n";
    std::cout << "Plik wejsciowy: " << cfg.inputFile << "\n";
    std::cout << "Plik wyjsciowy: " << cfg.outputFile << "\n";
    std::cout << "Powtorzenia: " << cfg.repetitions << "\n";
    std::cout << "Rozmiar instancji: " << cfg.instanceSize << "\n";
    std::cout << "-----------------------------\n";

    std::vector<std::vector<int>> matrix;

    if (cfg.useGenerator) {
        std::cout << "Tryb: Generowanie losowej macierzy.\n";
        std::cout << "Rozmiar instancji: " << cfg.instanceSize << "x" << cfg.instanceSize << "\n\n";
        matrix = generateRandomMatrix(cfg.instanceSize);
        printMatrix(matrix) ; // zakomentowac przed oddaniem, sluzy tylko dla testpowania generatora
    } else {
        std::cout << "Tryb: Wczytywanie macierzy z pliku.\n";
        std::cout << "Plik wejsciowy: " << cfg.inputFile << "\n";
        matrix = loadMatrix(cfg.inputFile);
        std::cout << "Pomyslnie wczytano macierz o rozmiarze: " << matrix.size() << "x" << matrix.size() << "\n\n";
    }
    
    TSP tsp(matrix);
    std::vector<int> best_path;
    int best_cost;

    std::ofstream outFile(cfg.outputFile, std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Blad: Nie mozna otworzyc pliku do zapisu CSV: " << cfg.outputFile << std::endl;
        return 1;
    }

    std::cout << "--- Rozpoczynamy testy algorytmow ---\n";

    // --- A. Brute Force ---
    double total_time_bf = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "Brute Force postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.bruteForce(best_path);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_bf += duration.count();
        outFile << "BruteForce," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "Brute Force: Koszt = " << best_cost << ", Sredni czas = " << total_time_bf / cfg.repetitions << " ms\n\n";

    // --- B. Nearest Neighbor ---
    double total_time_nn = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "Nearest Neighbor postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.nearestNeighbor(best_path); 
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_nn += duration.count();
        outFile << "NearestNeighbor," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "Nearest Neighbor: Koszt = " << best_cost << ", Sredni czas = " << total_time_nn / cfg.repetitions << " ms\n\n";

    // --- C. Random ---
    double total_time_rnd = 0;
    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) std::cout << "Random postep: " << i + 1 << "/" << cfg.repetitions << "\r" << std::flush;
        auto start = std::chrono::high_resolution_clock::now();
        best_cost = tsp.randomWalk(best_path);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        total_time_rnd += duration.count();
        outFile << "Random," << matrix.size() << "," << i + 1 << "," << best_cost << "," << duration.count() << "\n";
    }
    if (cfg.showProgress) std::cout << "\n";
    std::cout << "Random: Koszt (z ostatniej proby) = " << best_cost << ", Sredni czas = " << total_time_rnd / cfg.repetitions << " ms\n";

    outFile.close();
    std::cout << "\nTesty zakonczone. Wyniki w: " << cfg.outputFile << "\n";

    std::cout << "========================================\n";
    std::cout << "Zajeta pamiec (RAM): " << getMemoryUsage() << " KB\n";
    std::cout << "========================================\n";

    return 0;
}