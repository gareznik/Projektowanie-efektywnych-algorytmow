#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "tsp.h"

struct Config {
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

                if (key == "input_file") cfg.inputFile = value;
                else if (key == "output_file") cfg.outputFile = value;
                else if (key == "repetitions") cfg.repetitions = std::stoi(value);
                else if (key == "instance_size") cfg.instanceSize = std::stoi(value);
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

int main() {
    std::string configPath = "config.txt"; 
    Config cfg = readConfig(configPath);

    std::cout << "--- Wczytana konfiguracja ---\n";
    std::cout << "Plik wejsciowy: " << cfg.inputFile << "\n";
    std::cout << "Plik wyjsciowy: " << cfg.outputFile << "\n";
    std::cout << "Powtorzenia: " << cfg.repetitions << "\n";
    std::cout << "Rozmiar instancji: " << cfg.instanceSize << "\n";
    std::cout << "-----------------------------\n";

    std::vector<std::vector<int>> matrix = loadMatrix(cfg.inputFile);
    
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
        best_cost = tsp.nearestNeighbor(best_path, 0); 
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

    return 0;
}