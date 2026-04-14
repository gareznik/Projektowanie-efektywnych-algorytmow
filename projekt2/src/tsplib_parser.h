#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

class TSPLibParser {
public:
    static std::vector<std::vector<int>> loadMatrix(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Blad: Nie mozna otworzyc pliku: " << filename << std::endl;
            exit(1);
        }

        std::string line;
        int dimension = 0;

        // Пропускаем пустые строки и BOM
        while (std::getline(file, line)) {
            if (line.size() >= 3 && (unsigned char)line[0] == 0xEF) line = line.substr(3);
            if (!line.empty() && line.find_first_not_of(" \r\n\t") != std::string::npos) break;
        }

        // Если нет ключевых слов TSPLIB - это простой формат (число N и матрица)
        if (line.find("NAME") == std::string::npos && line.find("TYPE") == std::string::npos && line.find("DIMENSION") == std::string::npos) {
            dimension = std::stoi(line);
            return parseSimpleMatrix(file, dimension);
        }

        // Иначе парсим TSPLIB
        std::string weight_type = "";
        file.seekg(0);
        while (std::getline(file, line)) {
            if (line.find("DIMENSION") != std::string::npos) {
                size_t pos = line.find(":");
                dimension = std::stoi(line.substr(pos + 1));
            } else if (line.find("EDGE_WEIGHT_TYPE") != std::string::npos) {
                size_t pos = line.find(":");
                weight_type = line.substr(pos + 1);
                weight_type.erase(0, weight_type.find_first_not_of(" \t"));
                weight_type.erase(weight_type.find_last_not_of(" \t\r\n") + 1);
            } else if (line.find("EDGE_WEIGHT_SECTION") != std::string::npos) {
                return parseSimpleMatrix(file, dimension);
            } else if (line.find("NODE_COORD_SECTION") != std::string::npos) {
                return parseCoordinates(file, dimension, weight_type);
            }
        }
        return {};
    }

private:
    static std::vector<std::vector<int>> parseSimpleMatrix(std::ifstream& file, int dimension) {
        std::vector<std::vector<int>> matrix(dimension, std::vector<int>(dimension));
        for (int i = 0; i < dimension; ++i) {
            for (int j = 0; j < dimension; ++j) {
                if (!(file >> matrix[i][j])) break;
            }
        }
        return matrix;
    }

    static std::vector<std::vector<int>> parseCoordinates(std::ifstream& file, int dimension, const std::string& type) {
        struct Node { int id; double x, y; };
        std::vector<Node> nodes(dimension);
        for (int i = 0; i < dimension; ++i) file >> nodes[i].id >> nodes[i].x >> nodes[i].y;

        std::vector<std::vector<int>> matrix(dimension, std::vector<int>(dimension, 0));
        for (int i = 0; i < dimension; ++i) {
            for (int j = 0; j < dimension; ++j) {
                if (i == j) matrix[i][j] = -1;
                else {
                    double xd = nodes[i].x - nodes[j].x;
                    double yd = nodes[i].y - nodes[j].y;
                    if (type == "ATT") {
                        double rij = std::sqrt((xd * xd + yd * yd) / 10.0);
                        int tij = (int)std::round(rij);
                        matrix[i][j] = (tij < rij) ? tij + 1 : tij;
                    } else { // EUC_2D и другие
                        matrix[i][j] = (int)std::round(std::sqrt(xd * xd + yd * yd));
                    }
                }
            }
        }
        return matrix;
    }
};