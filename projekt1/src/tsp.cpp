#include "tsp.h"
#include <numeric>
#include <algorithm>
#include <random>
#include <climits>

// konstruktor klasy TSP, inicjalizuje macierz i liczbę wierzchołków
TSP::TSP(const std::vector<std::vector<int>>& adj_matrix) : matrix(adj_matrix) {
    num_vertices = matrix.size();
}

// funkcja pomocnicza
int TSP::calculatePathCost(const std::vector<int>& path) {
    int cost = 0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        cost += matrix[path[i]][path[i+1]];
    }
    cost += matrix[path.back()][path[0]]; 
    return cost;
}

// 1 -brute force
int TSP::bruteForce(std::vector<int>& best_path) {
    std::vector<int> current_path(num_vertices);
    std::iota(current_path.begin(), current_path.end(), 0);
    
    int min_cost = INT_MAX;
    best_path = current_path;

    do {
        int current_cost = calculatePathCost(current_path);
        if (current_cost < min_cost) {
            min_cost = current_cost;
            best_path = current_path;
        }
    } while (std::next_permutation(current_path.begin() + 1, current_path.end()));

    return min_cost;
}

// 2 - nearest neighbor
int TSP::nearestNeighbor(std::vector<int>& best_path, int start_vertex) {
    std::vector<bool> visited(num_vertices, false);
    best_path.clear();
    
    int current_vertex = start_vertex;
    best_path.push_back(current_vertex);
    visited[current_vertex] = true;
    
    int total_cost = 0;

    for (int step = 1; step < num_vertices; ++step) {
        int next_vertex = -1;
        int min_edge = INT_MAX;

        for (int i = 0; i < num_vertices; ++i) {
            if (!visited[i] && matrix[current_vertex][i] != -1 && matrix[current_vertex][i] < min_edge) {
                min_edge = matrix[current_vertex][i];
                next_vertex = i; // bierzemy pierwszego napotkanego najbliższego sąsiada (nie szukamy dalej, jeśli znajdziemy lepszego)
            }
        }
        if (next_vertex == -1) break;

        visited[next_vertex] = true;
        best_path.push_back(next_vertex);
        total_cost += min_edge;
        current_vertex = next_vertex;
    }
    total_cost += matrix[current_vertex][start_vertex]; 
    return total_cost;
}

// 3 - rrekurencyjna metoda najbliższych sąsiadów rnn
void TSP::rnn_dfs(int current_vertex, std::vector<bool>& visited, std::vector<int>& current_path, 
                  int current_cost, int& best_cost, std::vector<int>& best_path, int start_vertex) {
    
    if (current_path.size() == num_vertices) {
        int final_cost = current_cost + matrix[current_vertex][start_vertex];
        if (final_cost < best_cost) {
            best_cost = final_cost;
            best_path = current_path;
        }
        return;
    }

    int min_edge = INT_MAX;
    std::vector<int> next_vertices; // lista dla miast z takim samym minimalnym kosztem

    for (int i = 0; i < num_vertices; ++i) {
        if (!visited[i] && matrix[current_vertex][i] != -1) {
            if (matrix[current_vertex][i] < min_edge) {
                min_edge = matrix[current_vertex][i];
                next_vertices.clear();
                next_vertices.push_back(i);
            } else if (matrix[current_vertex][i] == min_edge) {
                //koszt jest taki samy, jak minimalny - zapamiętujemy też to miasto
                next_vertices.push_back(i);
            }
        }
    }

    if (next_vertices.empty()) return;

    // rekurencyjnie sprawdzamy wszystkie miasta z minimalnym kosztem
    for (int next_v : next_vertices) {
        visited[next_v] = true;
        current_path.push_back(next_v);
        
        rnn_dfs(next_v, visited, current_path, current_cost + min_edge, best_cost, best_path, start_vertex);
        
        visited[next_v] = false;
        current_path.pop_back();
    }
}

// 3.1 - wywolanie rekurencyjnej metody najbliższych sąsiadów (rnn)
int TSP::repetitiveNearestNeighbor(std::vector<int>& best_overall_path) {
    int global_best_cost = INT_MAX;

    // próbujemy jako startowe miasto każde z dostępnych, aby znaleźć najlepszy możliwy wynik
    for (int start_vertex = 0; start_vertex < num_vertices; ++start_vertex) {
        std::vector<bool> visited(num_vertices, false);
        std::vector<int> current_path;
        std::vector<int> local_best_path;
        int local_best_cost = INT_MAX;
        
        visited[start_vertex] = true;
        current_path.push_back(start_vertex);
        
        // wywołujemy rekurencyjną metodę najbliższych sąsiadów dla danego miasta startowego
        rnn_dfs(start_vertex, visited, current_path, 0, local_best_cost, local_best_path, start_vertex);
        
        // aktualizujemy globalny najlepszy wynik, jeśli lokalny jest lepszy
        if (local_best_cost < global_best_cost) {
            global_best_cost = local_best_cost;
            best_overall_path = local_best_path;
        }
    }
    
    return global_best_cost;
}

// 4 - random walk
int TSP::randomWalk(std::vector<int>& best_path, int iterations) {
    int global_min_cost = INT_MAX;
    std::vector<int> current_path(num_vertices);
    std::iota(current_path.begin(), current_path.end(), 0);
    
    std::random_device rd;
    std::mt19937 g(rd());

    for (int i = 0; i < iterations; ++i) {
        // zawsze startujemy z miasta z indeksem 0, a reszta jest losowa
        std::shuffle(current_path.begin() + 1, current_path.end(), g);
        int cost = calculatePathCost(current_path);
        
        if (cost < global_min_cost) {
            global_min_cost = cost;
            best_path = current_path;
        }
    }
    return global_min_cost;
}