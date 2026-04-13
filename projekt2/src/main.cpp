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

struct Config {
    int useGenerator = 1; 
    std::string inputFile;
    std::string outputFile;
    int repetitions = 1;
    int instanceSize = 0;
    int timeLimitS = 300; 
    bool showProgress = false;
};

Config readConfig(const std::string& filename) {
    Config cfg;
    std::ifstream file(filename);
    if (!file.is_open()) return cfg;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '\r') continue;
        std::istringstream is_line(line);
        std::string key, value;
        if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {
            if (!value.empty() && value.back() == '\r') value.pop_back();

            if (key == "use_generator") cfg.useGenerator = std::stoi(value);
            else if (key == "input_file") cfg.inputFile = value;
            else if (key == "output_file") cfg.outputFile = value;
            else if (key == "repetitions") cfg.repetitions = std::stoi(value);
            else if (key == "instance_size") cfg.instanceSize = std::stoi(value);
            else if (key == "time_limit_s") cfg.timeLimitS = std::stoi(value);
            else if (key == "show_progress") cfg.showProgress = (value == "1");
        }
    }
    return cfg;
}

std::vector<std::vector<int>> generateRandomATSP(int size) {
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            matrix[i][j] = (i == j) ? -1 : dist(gen);
    return matrix;
}

std::vector<std::vector<int>> generateRandomSTSP(int size) {
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);
    for (int i = 0; i < size; ++i) {
        for (int j = i; j < size; ++j) {
            if (i == j) matrix[i][j] = -1;
            else {
                int cost = dist(gen);
                matrix[i][j] = cost;
                matrix[j][i] = cost; 
            }
        }
    }
    return matrix;
}

SIZE_T getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024; // KB
    }
    return 0;
}

int main() {
    SetProcessAffinityMask(GetCurrentProcess(), 1); 
    
    Config cfg = readConfig("config.txt");
    std::vector<std::vector<int>> matrix;

    if (cfg.useGenerator == 1) matrix = generateRandomATSP(cfg.instanceSize);
    else if (cfg.useGenerator == 2) matrix = generateRandomSTSP(cfg.instanceSize);
    else matrix = TSPLibParser::loadMatrix(cfg.inputFile); 

    std::cout << "========================================\n";
    std::cout << "Rozmiar instancji: " << matrix.size() << "x" << matrix.size() << "\n";
    std::cout << "Limit czasu: " << cfg.timeLimitS << " sekund\n";
    std::cout << "Plik: " << cfg.inputFile << "\n";
    std::cout << "========================================\n\n";

    // Zapis nagłówka w CSV, jeśli plik jest pusty
    std::ifstream checkFile(cfg.outputFile);
    bool fileExists = checkFile.good();
    checkFile.close();

    std::ofstream outFile(cfg.outputFile, std::ios::app);
    if (!outFile.is_open()) return 1;
    
    if (!fileExists) {
        outFile << "Algorithm,Size,Instance,Cost,Time_ms,Memory_KB\n";
    }

    BranchAndBound bnb(matrix, cfg.timeLimitS);
    std::string algos[] = {"", "BFS", "DFS", "LowestCost"};

    for (int strategy = 1; strategy <= 3; ++strategy) {
        double total_time = 0;
        int result_cost = 0;
        bool timeout = false;

        for (int i = 0; i < cfg.repetitions; ++i) {
            SIZE_T mem_before = getMemoryUsage();
            
            auto start = std::chrono::high_resolution_clock::now();
            result_cost = bnb.solve(strategy);
            auto end = std::chrono::high_resolution_clock::now();
            
            SIZE_T mem_after = getMemoryUsage();
            std::chrono::duration<double, std::milli> duration = end - start;
            
            if (result_cost == -1) {
                timeout = true;
                std::cout << algos[strategy] << " -> TIMEOUT (> " << cfg.timeLimitS << "s)\n";
                outFile << algos[strategy] << "," << matrix.size() << "," << i + 1 << ",TIMEOUT,> " << cfg.timeLimitS * 1000 << "," << (mem_after-mem_before) << "\n";
                break; 
            } else {
                total_time += duration.count();
                outFile << algos[strategy] << "," << matrix.size() << "," << i + 1 << "," << result_cost << "," << duration.count() << "," << (mem_after-mem_before) << "\n";
            }
        }
        
        if (!timeout) {
            std::cout << algos[strategy] << ": Koszt = " << result_cost << ", Sredni czas = " << total_time / cfg.repetitions << " ms\n";
        }
    }

    outFile.close();
    return 0;
}