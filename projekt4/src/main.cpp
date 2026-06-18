#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <windows.h>
#include <psapi.h>

#include "tsplib_parser.h"
#include "aco.h" 

using namespace std;

struct Config {
    int useGenerator = 0; 
    int instanceSize = 10; 
    string inputFile;
    string outputFile;
    int repetitions = 1;
    int timeLimitS = 300;
    bool showProgress = false;
    
    // Parametry dla algorytmu mrowkowego
    int numAnts = 20;
    double alpha = 1.0;
    double beta = 3.0;
    double rho = 0.5;
    double qVal = 100.0;
    int saveTrace = 0;    
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
            else if (key == "num_ants") cfg.numAnts = stoi(value);
            else if (key == "alpha") cfg.alpha = stod(value);
            else if (key == "beta") cfg.beta = stod(value);
            else if (key == "rho") cfg.rho = stod(value);
            else if (key == "q_val") cfg.qVal = stod(value);
            else if (key == "save_trace") cfg.saveTrace = stoi(value);
        }
    }
    return file.close(), cfg;
}

vector<vector<int>> generateMatrix(int size, int type) {
    vector<vector<int>> matrix(size, vector<int>(size));
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 1000); 

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (i == j) matrix[i][j] = -1; 
            else {
                if (type == 1) matrix[i][j] = dist(gen);
                else if (type == 2) { 
                    if (i < j) matrix[i][j] = dist(gen);
                    else matrix[i][j] = matrix[j][i];
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

// Baza danych optimumow (OPT) pobrana z TSPLIB
int getOptimalCost(const string& filename) {
    static unordered_map<string, int> opt_values = {
        // Grafy male (0% bledu)
        {"burma14.tsp", 3323}, {"gr21.tsp", 2707}, {"ulysses22.tsp", 7013}, {"fri26.tsp", 937},
        
        // Grafy srednie (<50% bledu)
        {"berlin52.tsp", 7542}, {"eil51.tsp", 426}, {"st70.tsp", 675}, {"pr76.tsp", 108159},
        {"gr120.tsp", 6942}, {"si175.tsp", 21407},
        
        // Grafy duze (<100% bledu)
        {"gil262.tsp", 2378}, {"a280.tsp", 2579}, {"pr439.tsp", 107217}, {"pcb442.tsp", 50778},
        
        // Grafy ogromne (<150% bledu)
        {"u574.tsp", 36905}, {"pa561.tsp", 2763}, {"rat783.tsp", 8806}, {"pr1002.tsp", 259045},
        {"d1291.tsp", 50801}, {"rl1304.tsp", 252948}, {"vm1748.tsp", 336556}, 
        {"u1817.tsp", 57201}, {"pr2392.tsp", 378032}
    };

    string base_filename = filename.substr(filename.find_last_of("/\\") + 1);
    if (opt_values.count(base_filename)) return opt_values[base_filename];
    return -1; 
}

int main() {
    SetProcessAffinityMask(GetCurrentProcess(), 1); 
    Config cfg = readConfig("config.txt");
    vector<vector<int>> matrix;
    string instName;

    if (cfg.useGenerator == 0) {
        matrix = TSPLibParser::loadMatrix(cfg.inputFile);
        instName = cfg.inputFile.substr(cfg.inputFile.find_last_of("/\\") + 1);
        cout << "Loaded TSPLIB: " << instName << " (" << matrix.size() << "x" << matrix.size() << ")\n";
    } else {
        matrix = generateMatrix(cfg.instanceSize, cfg.useGenerator);
        instName = (cfg.useGenerator == 1 ? "GEN_ATSP_" : "GEN_STSP_") + to_string(cfg.instanceSize);
        cout << "Generated: " << instName << " (" << matrix.size() << "x" << matrix.size() << ")\n";
    }

    int target_opt = getOptimalCost(cfg.inputFile);
    if (target_opt > 0) cout << "Known Global Optimum (OPT): " << target_opt << "\n";
    else cout << "Optimum unknown for this instance.\n";

    ofstream outFile(cfg.outputFile, ios::app);
    outFile.seekp(0, ios::end);
    if (outFile.tellp() == 0) {
        outFile << "Instance,Size,Repetition,Ants,Alpha,Beta,Rho,Q,OPT,BestCost,Error(%),Time(ms),Memory(KB)\n";
    }

    AntColonyOptimization aco(matrix);

    for (int i = 0; i < cfg.repetitions; ++i) {
        if (cfg.showProgress) cout << "Postep: " << i + 1 << "/" << cfg.repetitions << "\r" << flush;

        vector<TracePoint> trace;
        auto start = chrono::high_resolution_clock::now();
        int initial_cost_for_run = 0;
        
        // Wywolanie algorytmu z przekazaniem target_opt dla Early Stopping
        int best_cost = aco.solve(cfg.timeLimitS, cfg.numAnts, cfg.alpha, cfg.beta, cfg.rho, cfg.qVal, 
                                  (cfg.saveTrace && i == 0) ? &trace : nullptr, &initial_cost_for_run, target_opt);
        
        auto end = chrono::high_resolution_clock::now();
        SIZE_T current_mem = getMemoryUsage();
        chrono::duration<double, milli> duration = end - start;

        double error_percent = 0.0;
        if (target_opt > 0) {
            error_percent = 100.0 * (best_cost - target_opt) / (double)target_opt;
        }

        outFile << instName << "," << matrix.size() << "," << i + 1 << "," 
                << cfg.numAnts << "," << cfg.alpha << "," << cfg.beta << "," << cfg.rho << "," << cfg.qVal << ","
                << target_opt << "," << best_cost << "," << fixed << setprecision(2) << error_percent << "," 
                << duration.count() << "," << current_mem << "\n";

        if (cfg.saveTrace && i == 0) {
            string traceFile = "output/trace_" + instName + ".csv";
            ofstream tFile(traceFile);
            tFile << "Time(ms),CurrentEpochBestCost,GlobalBestCost\n";
            for (const auto& tp : trace) {
                tFile << tp.time_ms << "," << tp.current_cost << "," << tp.best_cost << "\n";
            }
            tFile.close();
        }
    }
    
    cout << "\nTesty zakonczone. Wyniki zapisane w: " << cfg.outputFile << "\n";
    return 0;
}