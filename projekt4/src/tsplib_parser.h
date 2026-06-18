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

        while (std::getline(file, line)) {
            if (line.size() >= 3 && (unsigned char)line[0] == 0xEF) line = line.substr(3);
            if (!line.empty() && line.find_first_not_of(" \r\n\t") != std::string::npos) break;
        }

        if (line.find("NAME") == std::string::npos && line.find("TYPE") == std::string::npos && line.find("DIMENSION") == std::string::npos) {
            dimension = std::stoi(line);
            return parseSimpleMatrix(file, dimension);
        }

        std::string weight_type = "";
        std::string weight_format = "FULL_MATRIX";
        file.seekg(0);
        while (std::getline(file, line)) {
            if (line.find("DIMENSION") != std::string::npos) {
                dimension = std::stoi(line.substr(line.find(":") + 1));
            } else if (line.find("EDGE_WEIGHT_TYPE") != std::string::npos) {
                weight_type = extractValue(line);
            } else if (line.find("EDGE_WEIGHT_FORMAT") != std::string::npos) {
                weight_format = extractValue(line);
            } else if (line.find("EDGE_WEIGHT_SECTION") != std::string::npos) {
                
                if (weight_format == "LOWER_DIAG_ROW") {
                    return parseLowerDiagRow(file, dimension);
                } else {
                    return parseSimpleMatrix(file, dimension);
                }
                
            } else if (line.find("NODE_COORD_SECTION") != std::string::npos) {
                return parseCoordinates(file, dimension, weight_type);
            }
        }
        return {};
    }

private:
    static std::string extractValue(const std::string& line) {
        std::string val = line.substr(line.find(":") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t\r\n") + 1);
        return val;
    }

    static std::vector<std::vector<int>> parseSimpleMatrix(std::ifstream& file, int dimension) {
        std::vector<std::vector<int>> matrix(dimension, std::vector<int>(dimension));
        for (int i = 0; i < dimension; ++i) {
            for (int j = 0; j < dimension; ++j) {
                if (!(file >> matrix[i][j])) break;
                if (i == j && matrix[i][j] == 0) matrix[i][j] = -1;
            }
        }
        return matrix;
    }

    static std::vector<std::vector<int>> parseLowerDiagRow(std::ifstream& file, int dimension) {
        std::vector<std::vector<int>> matrix(dimension, std::vector<int>(dimension, 0));
        for (int i = 0; i < dimension; ++i) {
            for (int j = 0; j <= i; ++j) {
                int val;
                if (file >> val) {
                    if (i == j) {
                        matrix[i][j] = -1;
                    } else {
                        matrix[i][j] = val; 
                        matrix[j][i] = val;
                    }
                }
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
                    } else { 
                        matrix[i][j] = (int)std::round(std::sqrt(xd * xd + yd * yd));
                    }
                }
            }
        }
        return matrix;
    }
};