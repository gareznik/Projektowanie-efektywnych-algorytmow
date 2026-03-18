#include "tsp.h"
#include <numeric>
#include <algorithm>
#include <random>
#include <climits>

// Konstruktor
TSP::TSP(const std::vector<std::vector<int>>& adj_matrix) : matrix(adj_matrix) {
    num_vertices = matrix.size();
}

// Funkcja pomocnicza
int TSP::calculatePathCost(const std::vector<int>& path) {
    int cost = 0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        cost += matrix[path[i]][path[i+1]];
    }
    cost += matrix[path.back()][path[0]]; 
    return cost;
}

// 1. Przegląd zupełny (Brute Force)
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

// 2. Metoda najbliższych sąsiadów (Nearest Neighbor)
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
                next_vertex = i;
            }
        }

        if (next_vertex == -1) break;

        visited[next_vertex] = true;
        best_path.push_back(next_vertex);
        total_cost += min_edge;
        current_vertex = next_vertex;
    }

    total_cost += matrix[current_vertex][best_path[0]]; 
    return total_cost;
}

// 3. Metoda losowa (Random Walk)
int TSP::randomWalk(std::vector<int>& best_path) {
    std::vector<int> current_path(num_vertices);
    std::iota(current_path.begin(), current_path.end(), 0);
    
    std::random_device rd;
    std::mt19937 g(rd());
    
    std::shuffle(current_path.begin() + 1, current_path.end(), g);
    
    best_path = current_path;
    return calculatePathCost(current_path);
}