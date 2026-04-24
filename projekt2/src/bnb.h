#pragma once
#include <vector>
#include <queue>
#include <stack>
#include <climits>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <random>
#include <numeric>

struct Node {
    std::vector<int> path;                
    std::vector<std::vector<int>> matrix; 
    int cost;                             
    int level;                            
    int vertex;                           

    bool operator>(const Node& other) const {
        return cost > other.cost; 
    }
};

class BranchAndBound {
private:
    std::vector<std::vector<int>> matrix;
    int num_vertices;
    int time_limit_s;
    bool show_progress;
    int upper_bound_strategy; // 0=INF, 1=Random, 2=NN, 3=RNN, 4=RNN z remisami
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    bool isTimeUp() {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_time;
        return elapsed.count() > time_limit_s;
    }

    int getRandomUpperBound(int iterations = 1000) {
        int min_cost = INT_MAX;
        std::vector<int> path(num_vertices);
        std::iota(path.begin(), path.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());

        for (int i = 0; i < iterations; ++i) {
            std::shuffle(path.begin() + 1, path.end(), g);
            int current_cost = 0;
            bool valid = true;
            for (int j = 0; j < num_vertices - 1; ++j) {
                if (matrix[path[j]][path[j+1]] == -1) { valid = false; break; }
                current_cost += matrix[path[j]][path[j+1]];
            }
            if (valid && matrix[path.back()][path[0]] != -1) {
                current_cost += matrix[path.back()][path[0]];
                min_cost = std::min(min_cost, current_cost);
            }
        }
        return min_cost;
    }

    int nearestNeighborLogic(int start_vertex, bool use_ties) {
        std::vector<bool> visited(num_vertices, false);
        int current = start_vertex;
        visited[current] = true;
        int total_cost = 0;

        for (int step = 1; step < num_vertices; ++step) {
            int next_v = -1;
            int min_edge = INT_MAX;
            for (int i = 0; i < num_vertices; ++i) {
                if (!visited[i] && matrix[current][i] != -1 && matrix[current][i] < min_edge) {
                    min_edge = matrix[current][i];
                    next_v = i;
                }
            }
            if (next_v == -1) return INT_MAX;
            visited[next_v] = true;
            total_cost += min_edge;
            current = next_v;
        }
        if (matrix[current][start_vertex] == -1) return INT_MAX;
        return total_cost + matrix[current][start_vertex];
    }

    int getNNUpperBound() {
        return nearestNeighborLogic(0, false);
    }

    int getRNNUpperBound() {
        int best = INT_MAX;
        for (int i = 0; i < num_vertices; ++i) {
            best = std::min(best, nearestNeighborLogic(i, false));
        }
        return best;
    }

    void rnn_dfs_logic(int current_v, std::vector<bool>& visited, int step, int current_cost, int& best_cost, int start_v) {
        if (step == num_vertices) {
            if (matrix[current_v][start_v] != -1) {
                best_cost = std::min(best_cost, current_cost + matrix[current_v][start_v]);
            }
            return;
        }
        int min_e = INT_MAX;
        std::vector<int> next_v;
        for (int i = 0; i < num_vertices; ++i) {
            if (!visited[i] && matrix[current_v][i] != -1) {
                if (matrix[current_v][i] < min_e) {
                    min_e = matrix[current_v][i];
                    next_v.clear();
                    next_v.push_back(i);
                } else if (matrix[current_v][i] == min_e) {
                    next_v.push_back(i);
                }
            }
        }
        for (int n : next_v) {
            visited[n] = true;
            rnn_dfs_logic(n, visited, step + 1, current_cost + min_e, best_cost, start_v);
            visited[n] = false;
        }
    }

    int getRNNWithTiesUpperBound() {
        int global_best = INT_MAX;
        for (int i = 0; i < num_vertices; ++i) {
            std::vector<bool> visited(num_vertices, false);
            visited[i] = true;
            int local_best = INT_MAX;
            rnn_dfs_logic(i, visited, 1, 0, local_best, i);
            global_best = std::min(global_best, local_best);
        }
        return global_best;
    }

    int getInitialUpperBound() {
        if (upper_bound_strategy == 1) return getRandomUpperBound();
        if (upper_bound_strategy == 2) return getNNUpperBound();
        if (upper_bound_strategy == 3) return getRNNUpperBound();
        if (upper_bound_strategy == 4) return getRNNWithTiesUpperBound();
        return INT_MAX;
    }

    int reduceMatrix(std::vector<std::vector<int>>& mat) {
        int reduction_cost = 0;
        int n = mat.size();

        for (int i = 0; i < n; ++i) {
            int row_min = INT_MAX;
            for (int j = 0; j < n; ++j) {
                if (mat[i][j] != -1 && mat[i][j] < row_min) row_min = mat[i][j];
            }
            if (row_min != INT_MAX && row_min > 0) {
                reduction_cost += row_min;
                for (int j = 0; j < n; ++j) if (mat[i][j] != -1) mat[i][j] -= row_min;
            }
        }

        for (int j = 0; j < n; ++j) {
            int col_min = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (mat[i][j] != -1 && mat[i][j] < col_min) col_min = mat[i][j];
            }
            if (col_min != INT_MAX && col_min > 0) {
                reduction_cost += col_min;
                for (int i = 0; i < n; ++i) if (mat[i][j] != -1) mat[i][j] -= col_min;
            }
        }
        return reduction_cost;
    }

public:
    BranchAndBound(const std::vector<std::vector<int>>& adj_matrix, int limit_s, bool progress, int ub_strategy) 
        : matrix(adj_matrix), num_vertices(adj_matrix.size()), time_limit_s(limit_s), show_progress(progress), upper_bound_strategy(ub_strategy) {}

    int solve(int strategy) {
        start_time = std::chrono::high_resolution_clock::now();
        int best_cost = getInitialUpperBound();
        
        Node root;
        root.path.push_back(0);
        root.level = 1;
        root.vertex = 0;
        root.matrix = matrix;
        root.cost = reduceMatrix(root.matrix);

        if (strategy == 1) return runBFS(root, best_cost);
        if (strategy == 2) return runDFS(root, best_cost);
        if (strategy == 3) return runLowestCost(root, best_cost);
        
        return -1;
    }

private:
    void createChild(const Node& parent, int next_v, int& best_cost, auto& container) {
        int edge_weight = parent.matrix[parent.vertex][next_v];
        Node child;
        child.path = parent.path;
        child.path.push_back(next_v);
        child.level = parent.level + 1;
        child.vertex = next_v;
        child.matrix = parent.matrix;

        for (int i = 0; i < num_vertices; ++i) {
            child.matrix[parent.vertex][i] = -1;
            child.matrix[i][next_v] = -1;
        }
        child.matrix[next_v][0] = -1;

        int reduction = reduceMatrix(child.matrix);
        child.cost = parent.cost + edge_weight + reduction;

        if (child.cost < best_cost) {
            if (child.level == num_vertices) {
                best_cost = child.cost;
                if (show_progress) std::cout << "  [New Best] " << best_cost << "\n";
            } else {
                container.push(child);
            }
        }
    }

    int runBFS(Node root, int best_cost) {
        std::queue<Node> q;
        q.push(root);
        while (!q.empty()) {
            if (isTimeUp()) return -1;
            Node current = q.front(); q.pop();
            if (current.cost >= best_cost) continue;
            for (int i = 0; i < num_vertices; ++i) {
                if (current.matrix[current.vertex][i] != -1) createChild(current, i, best_cost, q);
            }
        }
        return best_cost;
    }

    int runDFS(Node root, int best_cost) {
        std::stack<Node> s;
        s.push(root);
        while (!s.empty()) {
            if (isTimeUp()) return -1;
            Node current = s.top(); s.pop();
            if (current.cost >= best_cost) continue;
            for (int i = num_vertices - 1; i >= 0; --i) {
                if (current.matrix[current.vertex][i] != -1) createChild(current, i, best_cost, s);
            }
        }
        return best_cost;
    }

    int runLowestCost(Node root, int best_cost) {
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
        pq.push(root);
        while (!pq.empty()) {
            if (isTimeUp()) return -1;
            Node current = pq.top(); pq.pop();
            if (current.cost >= best_cost) continue;
            for (int i = 0; i < num_vertices; ++i) {
                if (current.matrix[current.vertex][i] != -1) createChild(current, i, best_cost, pq);
            }
        }
        return best_cost;
    }
};