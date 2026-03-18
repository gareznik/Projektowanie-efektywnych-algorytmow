#pragma once
#include <vector>

class TSP {
private:
    std::vector<std::vector<int>> matrix;
    int num_vertices;
    int calculatePathCost(const std::vector<int>& path);

public:
    TSP(const std::vector<std::vector<int>>& adj_matrix);
    
    int bruteForce(std::vector<int>& best_path);
    int nearestNeighbor(std::vector<int>& best_path, int start_vertex = 0);
    int randomWalk(std::vector<int>& best_path);
};