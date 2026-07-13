#pragma once
#include <iostream>
#include <iomanip>
#include <glm/glm.hpp>


void printMatrix(const std::string& name, const glm::mat4& m) {
    std::cout << "=== " << name << " ===" << std::endl;
    for (int row = 0; row < 4; ++row) {
        std::cout << "[ ";
        for (int col = 0; col < 4; ++col) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << m[col][row] << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << "==================\n" << std::endl;
}