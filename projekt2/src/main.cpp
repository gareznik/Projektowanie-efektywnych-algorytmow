#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <windows.h>
#include <psapi.h>

#include "tsplib_parser.h"
#include "bnb.h"
#include <random>

struct Config {
    int useGenerator = 0;
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
            else { int c = dist(gen); matrix[i][j] = c; matrix[j][i] = c; }
        }
    }
    return matrix;
}

SIZE_T getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    return GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)) ? pmc.WorkingSetSize / 1024 : 0;
}

int main() {
    SetProcessAffinityMask(GetCurrentProcess(), 1); 
    Config cfg = readConfig("config.txt");
    std::vector<std::vector<int>> matrix;

    if (cfg.useGenerator == 1) matrix = generateRandomATSP(cfg.instanceSize);
    else if (cfg.useGenerator == 2) matrix = generateRandomSTSP(cfg.instanceSize);
    else matrix = TSPLibParser::loadMatrix(cfg.inputFile);

    if (matrix.empty()) {
        std::cerr << "Blad: Nie udalo sie zaladowac macierzy!\n";
        return 1;
    }

    std::ofstream outFile(cfg.outputFile, std::ios::app);
    BranchAndBound bnb(matrix, cfg.timeLimitS);
    std::string algos[] = {"", "BFS", "DFS", "LowestCost"};

    for (int s = 1; s <= 3; ++s) {
        double time_sum = 0;
        for (int i = 0; i < cfg.repetitions; ++i) {
            SIZE_T mem_b = getMemoryUsage();
            auto start = std::chrono::high_resolution_clock::now();
            int cost = bnb.solve(s);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> dur = end - start;

            if (cost == -1) {
                std::cout << algos[s] << " -> TIMEOUT\n";
                outFile << algos[s] << "," << matrix.size() << "," << i+1 << ",TIMEOUT\n";
                break;
            }
            time_sum += dur.count();
            outFile << algos[s] << "," << matrix.size() << "," << i+1 << "," << cost << "," << dur.count() << "," << getMemoryUsage() - mem_b << "\n";
        }
    }
    return 0;
}