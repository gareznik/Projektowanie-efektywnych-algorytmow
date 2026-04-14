#pragma once
#include <vector>
#include <queue>
#include <stack>
#include <climits>
#include <chrono>
#include <algorithm>

struct Node {
    std::vector<int> path;
    int cost;
    int level;

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
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    bool isTimeUp() {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_time;
        return elapsed.count() > time_limit_s;
    }

    int getInitialUpperBound() {
        std::vector<bool> visited(num_vertices, false);
        int current_vertex = 0, total_cost = 0;
        visited[0] = true;

        for (int step = 1; step < num_vertices; ++step) {
            int next_vertex = -1;
            int min_edge = INT_MAX;
            for (int i = 0; i < num_vertices; ++i) {
                if (!visited[i] && matrix[current_vertex][i] != -1 && matrix[current_vertex][i] < min_edge) {
                    min_edge = matrix[current_vertex][i];
                    next_vertex = i;
                }
            }
            if (next_vertex == -1) return INT_MAX;
            visited[next_vertex] = true;
            total_cost += min_edge;
            current_vertex = next_vertex;
        }
        return total_cost + matrix[current_vertex][0];
    }

public:
    BranchAndBound(const std::vector<std::vector<int>>& adj_matrix, int limit_s, bool progress) 
        : matrix(adj_matrix), num_vertices(adj_matrix.size()), time_limit_s(limit_s), show_progress(progress) {}

    int solve(int strategy) {
        start_time = std::chrono::high_resolution_clock::now();
        int best_cost = getInitialUpperBound(); 
        
        Node root;
        root.path.push_back(0);
        root.cost = 0;
        root.level = 1;

        if (strategy == 1) return runBFS(root, best_cost);
        if (strategy == 2) return runDFS(root, best_cost);
        if (strategy == 3) return runLowestCost(root, best_cost);
        
        return -1;
    }

private:
    int runBFS(Node root, int best_cost) {
        std::queue<Node> q;
        q.push(root);
        while (!q.empty()) {
            if (isTimeUp()) return -1;
            Node current = q.front();
            q.pop();

            if (current.level == num_vertices) {
                int final_cost = current.cost + matrix[current.path.back()][0];
                if (final_cost < best_cost) {
                    best_cost = final_cost;
                    if (show_progress) {
                        std::cout << "  [Progress] Znaleziono lepszą transe! Nowy koszt: " << best_cost << "\n";
                    }
                }
                continue;
            }

            for (int i = 0; i < num_vertices; ++i) {
                if (matrix[current.path.back()][i] != -1 && std::find(current.path.begin(), current.path.end(), i) == current.path.end()) {
                    Node child = current;
                    child.path.push_back(i);
                    child.cost += matrix[current.path.back()][i];
                    child.level++;
                    if (child.cost < best_cost) q.push(child);
                }
            }
        }
        return best_cost;
    }

    int runDFS(Node root, int best_cost) {
        std::stack<Node> s;
        s.push(root);
        while (!s.empty()) {
            if (isTimeUp()) return -1;
            Node current = s.top();
            s.pop();

            if (current.level == num_vertices) {
                int final_cost = current.cost + matrix[current.path.back()][0];
                if (final_cost < best_cost) {
                    best_cost = final_cost;
                    if (show_progress) {
                        std::cout << "  [Progress] Znaleziono lepszą transe! Nowy koszt: " << best_cost << "\n";
                    }
                }
                continue;
            }

            for (int i = num_vertices - 1; i >= 0; --i) {
                if (matrix[current.path.back()][i] != -1 && std::find(current.path.begin(), current.path.end(), i) == current.path.end()) {
                    Node child = current;
                    child.path.push_back(i);
                    child.cost += matrix[current.path.back()][i];
                    child.level++;
                    if (child.cost < best_cost) s.push(child);
                }
            }
        }
        return best_cost;
    }

    int runLowestCost(Node root, int best_cost) {
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
        pq.push(root);
        while (!pq.empty()) {
            if (isTimeUp()) return -1;
            Node current = pq.top();
            pq.pop();

            if (current.level == num_vertices) {
                int final_cost = current.cost + matrix[current.path.back()][0];
                if (final_cost < best_cost) {
                    best_cost = final_cost;
                    if (show_progress) {
                        std::cout << "  [Progress] Znaleziono lepszą transe! Nowy koszt: " << best_cost << "\n";
                    }
                }
                continue;
            }

            for (int i = 0; i < num_vertices; ++i) {
                if (matrix[current.path.back()][i] != -1 && std::find(current.path.begin(), current.path.end(), i) == current.path.end()) {
                    Node child = current;
                    child.path.push_back(i);
                    child.cost += matrix[current.path.back()][i];
                    child.level++;
                    if (child.cost < best_cost) pq.push(child);
                }
            }
        }
        return best_cost;
    }
};