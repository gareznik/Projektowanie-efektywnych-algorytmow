#pragma once
#include <vector>

class TSP {
private:
    std::vector<std::vector<int>> matrix;
    int num_vertices;
    int calculatePathCost(const std::vector<int>& path);

    // rekurencyjna metoda najbliższych sąsiadów (RNN)
    void rnn_dfs(int current_vertex, std::vector<bool>& visited, std::vector<int>& current_path, 
                 int current_cost, int& best_cost, std::vector<int>& best_path, int start_vertex);

public:
    TSP(const std::vector<std::vector<int>>& adj_matrix);
    
    // brute force
    int bruteForce(std::vector<int>& best_path);
    
    // standardowy nn
    int nearestNeighbor(std::vector<int>& best_path, int start_vertex = 0);
    
    // rnn
    int repetitiveNearestNeighbor(std::vector<int>& best_path);
    
    // rand
    int randomWalk(std::vector<int>& best_path, int iterations = 1000);
};