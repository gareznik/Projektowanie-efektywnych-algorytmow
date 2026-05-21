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
#include "sa.h"

using namespace std;

struct Config {
    int useGenerator = 0; // 0 - из файла, 1 - ATSP, 2 - STSP
    int instanceSize = 10; // Размер генерируемой матрицы
    string inputFile;
    string outputFile;
    int repetitions = 1;
    int timeLimitS = 300;
    bool showProgress = false;
    
    // Параметры для Отжига
    int epochLength = 100;
    double alpha = 0.99;
    int coolingScheme = 0; 
    int useUB = 1;         
    double initialTemp = 0.0; 

    int saveTrace = 0; // 0 - не сохранять, 1 - сохранять   
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
        string key, value;
        if (getline(is_line, key, '=') && getline(is_line, value)) {
            if (!value.empty() && value.back() == '\r') value.pop_back();
            if (key == "use_generator") cfg.useGenerator = stoi(value);
            else if (key == "instance_size") cfg.instanceSize = stoi(value);
            else if (key == "input_file") cfg.inputFile = value;
            else if (key == "output_file") cfg.outputFile = value;
            else if (key == "repetitions") cfg.repetitions = stoi(value);
            else if (key == "time_limit_s") cfg.timeLimitS = stoi(value);
            else if (key == "show_progress") cfg.showProgress = (value == "1");
            else if (key == "epoch_length") cfg.epochLength = stoi(value);
            else if (key == "alpha") cfg.alpha = stod(value);
            else if (key == "cooling_scheme") cfg.coolingScheme = stoi(value);
            else if (key == "use_ub") cfg.useUB = stoi(value);
            else if (key == "initial_temp") cfg.initialTemp = stod(value);
        }
    }
    return cfg;
}

// Генератор графов
vector<vector<int>> generateMatrix(int size, int type) {
    vector<vector<int>> matrix(size, vector<int>(size));
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 1000); // Дистанция от 1 до 1000

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (i == j) {
                matrix[i][j] = -1; // Расстояние до самого себя
            } else {
                if (type == 1) { // ATSP (асимметричный)
                    matrix[i][j] = dist(gen);
                } else if (type == 2) { // STSP (симметричный)
                    if (i < j) {
                        matrix[i][j] = dist(gen);
                    } else {
                        matrix[i][j] = matrix[j][i];
                    }
                }
            }
        }
    }
    return matrix;
}

SIZE_T getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize / 1024;
    }
    return 0;
}

int main() {
    SetProcessAffinityMask(GetCurrentProcess(), 1); 
    Config cfg = readConfig("config.txt");
    vector<vector<int>> matrix;

    string instName;

    // Выбор: Файл или Генератор
    if (cfg.useGenerator == 0) {
        matrix = TSPLibParser::loadMatrix(cfg.inputFile);
        instName = cfg.inputFile.substr(cfg.inputFile.find_last_of("/\\") + 1);
        cout << "Loaded TSPLIB: " << instName << " (" << matrix.size() << "x" << matrix.size() << ")\n";
    } else {
        matrix = generateMatrix(cfg.instanceSize, cfg.useGenerator);
        instName = (cfg.useGenerator == 1 ? "GEN_ATSP_" : "GEN_STSP_") + to_string(cfg.instanceSize);
        cout << "Generated: " << instName << " (" << matrix.size() << "x" << matrix.size() << ")\n";
    }

    ofstream outFile(cfg.outputFile, ios::app);
    outFile.seekp(0, ios::end);
    if (outFile.tellp() == 0) {
        outFile << "Instance,Size,Repetition,CoolingScheme,UseUB,EpochLen,Alpha,InitTemp,LB(MST),InitCost(UB),BestCost,Time(ms),Memory(KB)\n";
    }

    SimulatedAnnealing sa(matrix);
    int lb_mst = sa.computeMSTLowerBound();
    int initial_cost = sa.getCost(cfg.useUB ? sa.generateRNNPath() : vector<int>(matrix.size(), 0));

    cout << "Lower Bound (MST): " << lb_mst << "\n";
    cout << "Starting cost (UB=" << cfg.useUB << "): " << initial_cost << "\n";

    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) cout << "Postep: " << i + 1 << "/" << cfg.repetitions << "\r" << flush;

        // НОВОЕ: Вектор для сбора следа (истории)
        vector<TracePoint> trace;
        
        auto start = chrono::high_resolution_clock::now();
        // Передаем указатель на trace только если это ПЕРВОЕ повторение (i==0) и флаг включен
        int best_cost = sa.solve(cfg.timeLimitS, cfg.epochLength, cfg.alpha, cfg.coolingScheme, cfg.useUB, cfg.initialTemp, (cfg.saveTrace && i == 0) ? &trace : nullptr);
        auto end = chrono::high_resolution_clock::now();
        
        SIZE_T current_mem = getMemoryUsage();
        chrono::duration<double, milli> duration = end - start;
        
        // Запись в главный results.csv (Остается без изменений!)
        outFile << instName << "," << matrix.size() << "," << i + 1 << "," 
                << (cfg.coolingScheme == 0 ? "Geometric" : "Linear") << "," 
                << (cfg.useUB ? "Yes" : "No") << "," 
                << cfg.epochLength << "," << cfg.alpha << "," << cfg.initialTemp << ","
                << lb_mst << "," << initial_cost << "," << best_cost << "," 
                << duration.count() << "," << current_mem << "\n";

        // НОВОЕ: Запись истории в ОТДЕЛЬНЫЙ файл (как просит препод)
        if (cfg.saveTrace && i == 0) {
            string traceFile = "output/trace_" + instName + ".csv";
            ofstream tFile(traceFile);
            tFile << "Time(ms),CurrentCost,BestCost,Temperature\n";
            for (const auto& tp : trace) {
                tFile << tp.time_ms << "," << tp.current_cost << "," << tp.best_cost << "," << tp.temperature << "\n";
            }
            tFile.close();
            if (cfg.showProgress) cout << "\nZapisano historie zbieznosci do: " << traceFile << "\n";
        }
    }
    
    cout << "\nTesty zakonczone. Wyniki zapisane w: " << cfg.outputFile << "\n";
    return 0;
}