#pragma once
#include <vector>
#include <numeric>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

struct TracePoint {
    double time_ms;
    int current_cost; 
    int best_cost;
};

class AntColonyOptimization {
private:
    const std::vector<std::vector<int>>& matrix;
    int n;
    std::mt19937 rng;

    std::vector<std::vector<double>> pheromone;
    std::vector<std::vector<double>> heuristic; // [i][j] = (1.0 / distance)^beta

public:
    AntColonyOptimization(const std::vector<std::vector<int>>& m) : matrix(m), n(m.size()) {
        std::random_device rd;
        rng.seed(rd());
    }

    // Obliczanie kosztu trasy
    int getCost(const std::vector<int>& path) {
        int cost = 0;
        for (int i = 0; i < n - 1; ++i) {
            cost += matrix[path[i]][path[i + 1]];
        }
        cost += matrix[path[n - 1]][path[0]];
        return cost;
    }

    // Pre-kalkulacja macierzy heurystyki (Senior Optimization)
    void initializeHeuristic(double beta) {
        heuristic.assign(n, std::vector<double>(n, 0.0));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j && matrix[i][j] > 0) {
                    heuristic[i][j] = std::pow(1.0 / matrix[i][j], beta);
                }
            }
        }
    }

    void initializePheromones(double initial_pheromone) {
        pheromone.assign(n, std::vector<double>(n, initial_pheromone));
    }

    // Glowna metoda ACO z Early Stopping
    int solve(int time_limit_s, int num_ants, double alpha, double beta, double rho, double q_val, 
              std::vector<TracePoint>* trace = nullptr, int* initial_guess = nullptr, int target_opt = -1) {
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Inicjalizacja heurystyki
        initializeHeuristic(beta);
        
        // Poczatkowy feromon bazowany na rozwiazaniu zachlannym (Nearest Neighbor)
        int nn_cost = generateRNNPathCost();
        if (initial_guess) *initial_guess = nn_cost;
        
        double tau0 = (double)num_ants / nn_cost; 
        initializePheromones(tau0);

        std::vector<int> global_best_path;
        int global_best_cost = std::numeric_limits<int>::max();

        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        std::uniform_int_distribution<int> city_dist(0, n - 1);

        int iteration = 0;

        while (true) {
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            if (elapsed.count() > time_limit_s) break;

            std::vector<std::vector<int>> ant_paths(num_ants, std::vector<int>(n));
            std::vector<int> ant_costs(num_ants);

            int iteration_best_cost = std::numeric_limits<int>::max();

            // 1. Budowanie tras przez mrówki
            for (int k = 0; k < num_ants; ++k) {
                std::vector<bool> visited(n, false);
                int start_city = city_dist(rng);
                ant_paths[k][0] = start_city;
                visited[start_city] = true;
                int current_city = start_city;

                for (int step = 1; step < n; ++step) {
                    std::vector<double> probabilities(n, 0.0);
                    double sum_prob = 0.0;

                    // Obliczanie prawdopodobienstw
                    for (int j = 0; j < n; ++j) {
                        if (!visited[j] && matrix[current_city][j] > 0) {
                            double tau = std::pow(pheromone[current_city][j], alpha);
                            double eta = heuristic[current_city][j]; 
                            probabilities[j] = tau * eta;
                            sum_prob += probabilities[j];
                        }
                    }

                    int next_city = -1;
                    if (sum_prob > 0.0) {
                        // Selekcja kolom ruletki
                        double random_val = prob_dist(rng) * sum_prob;
                        double cumulative = 0.0;
                        for (int j = 0; j < n; ++j) {
                            if (!visited[j] && probabilities[j] > 0) {
                                cumulative += probabilities[j];
                                if (cumulative >= random_val) {
                                    next_city = j;
                                    break;
                                }
                            }
                        }
                    } 
                    
                    if (next_city == -1) { // Fallback
                        for (int j = 0; j < n; ++j) {
                            if (!visited[j]) { next_city = j; break; }
                        }
                    }

                    ant_paths[k][step] = next_city;
                    visited[next_city] = true;
                    current_city = next_city;
                }

                ant_costs[k] = getCost(ant_paths[k]);

                if (ant_costs[k] < iteration_best_cost) {
                    iteration_best_cost = ant_costs[k];
                }

                if (ant_costs[k] < global_best_cost) {
                    global_best_cost = ant_costs[k];
                    global_best_path = ant_paths[k];
                }
            }

            // 2. Parowanie feromonow
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    pheromone[i][j] *= (1.0 - rho);
                    if(pheromone[i][j] < 1e-6) pheromone[i][j] = 1e-6; // Zapobieganie stagnacji
                }
            }

            // 3. Deponowanie feromonow przez wszystkie mrowki
            for (int k = 0; k < num_ants; ++k) {
                double delta_tau = q_val / ant_costs[k];
                for (int i = 0; i < n - 1; ++i) {
                    int u = ant_paths[k][i];
                    int v = ant_paths[k][i + 1];
                    pheromone[u][v] += delta_tau;
                    pheromone[v][u] += delta_tau; 
                }
                int u = ant_paths[k][n - 1];
                int v = ant_paths[k][0];
                pheromone[u][v] += delta_tau;
                pheromone[v][u] += delta_tau;
            }

            // Elitaryzm - wzmocnienie najlepszej globalnej trasy
            double delta_tau_elite = (q_val / global_best_cost) * 2.0;
            for (int i = 0; i < n - 1; ++i) {
                int u = global_best_path[i];
                int v = global_best_path[i + 1];
                pheromone[u][v] += delta_tau_elite;
                pheromone[v][u] += delta_tau_elite;
            }
            pheromone[global_best_path[n - 1]][global_best_path[0]] += delta_tau_elite;
            pheromone[global_best_path[0]][global_best_path[n - 1]] += delta_tau_elite;

            // Logowanie trace
            if (trace != nullptr && iteration % 10 == 0) {
                auto trace_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed_ms = trace_time - start_time;
                trace->push_back({elapsed_ms.count(), iteration_best_cost, global_best_cost});
            }

            // --- EARLY STOPPING ---
            // Jesli znamy OPT i znalezlismy sciezke idealna, przerywamy natychmiast!
            if (target_opt > 0 && global_best_cost <= target_opt) {
                break;
            }

            iteration++;
        }

        return global_best_cost;
    }

private:
    int generateRNNPathCost() {
        int best_cost = std::numeric_limits<int>::max();
        for (int start = 0; start < std::min(n, 10); ++start) {
            std::vector<bool> visited(n, false);
            int current = start;
            visited[current] = true;
            int current_cost = 0;

            for (int i = 1; i < n; ++i) {
                int next = -1;
                int min_dist = 1e9;
                for (int j = 0; j < n; ++j) {
                    if (!visited[j] && matrix[current][j] > 0 && matrix[current][j] < min_dist) {
                        min_dist = matrix[current][j];
                        next = j;
                    }
                }
                if (next == -1) {
                    for (int j = 0; j < n; ++j) {
                        if (!visited[j]) { next = j; break; }
                    }
                }
                current_cost += matrix[current][next];
                visited[next] = true;
                current = next;
            }
            current_cost += matrix[current][start];
            if (current_cost < best_cost) best_cost = current_cost;
        }
        return best_cost;
    }
};