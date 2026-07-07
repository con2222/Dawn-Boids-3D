#include <iostream>

#include "App.hpp" 



int main() {
    WGPUBoids::App app;
    
    if (!app.init(1920, 1080, "Boids Simulation")) {
        std::cerr << "Can't initialize app" << std::endl;
        return EXIT_FAILURE;
    }
    
    app.run();

    return EXIT_SUCCESS;
}