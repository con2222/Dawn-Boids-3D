#include "App.hpp" 

#include <iostream>


int main() {
    WGPUBoids::App app;
    
    if (!app.init("Boids Simulation")) {
        std::cerr << "Can't initialize app" << std::endl;
        return EXIT_FAILURE;
    }
    
    app.run();

    return EXIT_SUCCESS;
}