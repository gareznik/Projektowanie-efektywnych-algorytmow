#pragma once
#include <vector>
#include <numeric>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <iostream>

class SimulatedAnnealing {
private:
    const std::vector<std::vector<int>>& matrix;
    int n;
    std::mt19937 rng;

public:
    SimulatedAnnealing(const std::vector<std::vector<int>>& m) : matrix(m), n(m.size()) {
        std::random_device rd;
        rng.seed(rd());
    }

    // Вычисление стоимости маршрута
    int getCost(const std::vector<int>& path) {
        int cost = 0;
        for (int i = 0; i < n - 1; ++i) {
            cost += matrix[path[i]][path[i + 1]];
        }
        cost += matrix[path[n - 1]][path[0]];
        return cost;
    }

   // Upper Bound (UB) - Repetitive Nearest Neighbor (RNN)
    std::vector<int> generateRNNPath() {
        std::vector<int> best_overall_path;
        int best_overall_cost = 2e9; // Условная бесконечность

        // Пробуем стартовать жадный алгоритм из КАЖДОГО города
        for (int start_city = 0; start_city < n; ++start_city) {
            std::vector<int> path(n);
            std::vector<bool> visited(n, false);
            int current = start_city;
            path[0] = current;
            visited[current] = true;

            for (int i = 1; i < n; ++i) {
                int next = -1;
                int min_dist = 1e9;
                for (int j = 0; j < n; ++j) {
                    if (!visited[j] && matrix[current][j] != -1 && matrix[current][j] < min_dist) {
                        min_dist = matrix[current][j];
                        next = j;
                    }
                }
                if (next == -1) { // Fallback, если граф не полносвязный
                    for (int j = 0; j < n; ++j) {
                        if (!visited[j]) { next = j; break; }
                    }
                }
                path[i] = next;
                visited[next] = true;
                current = next;
            }

            // Оцениваем получившийся маршрут
            int current_cost = getCost(path);
            
            // Запоминаем, если этот маршрут лучше предыдущих
            if (current_cost < best_overall_cost) {
                best_overall_cost = current_cost;
                best_overall_path = path;
            }
        }
        return best_overall_path;
    }
    // Lower Bound (LB) - Минимальное остовное дерево (MST, алгоритм Прима)
    int computeMSTLowerBound() {
        std::vector<int> min_e(n, 1e9);
        std::vector<bool> in_mst(n, false);
        min_e[0] = 0;
        int mst_cost = 0;

        for (int i = 0; i < n; ++i) {
            int u = -1;
            for (int j = 0; j < n; ++j) {
                if (!in_mst[j] && (u == -1 || min_e[j] < min_e[u])) u = j;
            }
            if (min_e[u] == 1e9) break; // Несвязный граф
            in_mst[u] = true;
            mst_cost += min_e[u];

            for (int v = 0; v < n; ++v) {
                if (!in_mst[v] && matrix[u][v] != -1) {
                    // Для ATSP берем минимальное ребро между (u,v) и (v,u)
                    int edge_weight = matrix[u][v];
                    if (matrix[v][u] != -1) {
                        edge_weight = std::min(edge_weight, matrix[v][u]);
                    }
                    if (edge_weight < min_e[v]) min_e[v] = edge_weight;
                }
            }
        }
        return mst_cost;
    }

    // Автоматический расчет начальной температуры (требование задания)
    // Проводим серию случайных мутаций и смотрим на среднее ухудшение результата
    double calculateInitialTemperature(const std::vector<int>& initial_path, double init_prob = 0.99) {
        int current_cost = getCost(initial_path);
        double delta_sum = 0.0;
        int positive_deltas = 0;
        int samples = 1000;

        std::uniform_int_distribution<int> dist(0, n - 1);

        for (int i = 0; i < samples; ++i) {
            std::vector<int> neighbor = initial_path;
            int u = dist(rng), v = dist(rng);
            std::swap(neighbor[u], neighbor[v]);
            
            int neighbor_cost = getCost(neighbor);
            if (neighbor_cost > current_cost) {
                delta_sum += (neighbor_cost - current_cost);
                positive_deltas++;
            }
        }

        if (positive_deltas == 0) return 1000.0; // Защита от деления на 0
        double avg_delta = delta_sum / positive_deltas;
        return -avg_delta / std::log(init_prob);
    }

    // Главный метод Отжига
    // cooling_scheme: 0 - Геометрическое (T *= alpha), 1 - Линейное (T -= alpha)
    int solve(int time_limit_s, int epoch_length, double alpha, int cooling_scheme, bool use_ub, double init_temp_param) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<int> current_path;
        if (use_ub) {
            current_path = generateRNNPath(); // <-- ТЕПЕРЬ ТУТ ИСПОЛЬЗУЕМ RNN
        } else {
            current_path.resize(n);
            std::iota(current_path.begin(), current_path.end(), 0);
            std::shuffle(current_path.begin(), current_path.end(), rng);
        }

        int current_cost = getCost(current_path);
        std::vector<int> best_path = current_path;
        int best_cost = current_cost;

        // Определяем начальную температуру: заданная вручную или вычисленная
        double T = init_temp_param > 0 ? init_temp_param : calculateInitialTemperature(current_path);
        double T_min = 0.001;

        std::uniform_int_distribution<int> dist(1, n - 1); // Не меняем позицию стартового города (0)
        std::uniform_real_distribution<double> prob(0.0, 1.0);

        while (T > T_min) {
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            if (elapsed.count() > time_limit_s) break; // Остановка по ограничению времени

            // Эпоха - количество итераций на одной температуре
            for (int i = 0; i < epoch_length; ++i) {
                std::vector<int> neighbor = current_path;
                int u = dist(rng), v = dist(rng);
                while (u == v) v = dist(rng);
                
                // Операция SWAP (обмен двух городов местами)
                std::swap(neighbor[u], neighbor[v]);
                int neighbor_cost = getCost(neighbor);
                int delta = neighbor_cost - current_cost;

                // Если решение лучше (delta < 0) ИЛИ срабатывает вероятность (отжиг)
                if (delta < 0 || prob(rng) < std::exp(-delta / T)) {
                    current_path = neighbor;
                    current_cost = neighbor_cost;

                    if (current_cost < best_cost) {
                        best_path = current_path;
                        best_cost = current_cost;
                    }
                }
            }

            // Охлаждение
            if (cooling_scheme == 0) {
                T *= alpha; // Геометрическое
            } else {
                T -= alpha; // Линейное
            }
        }

        return best_cost;
    }
};