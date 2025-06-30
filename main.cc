#include "ledmgr.h"
#include "led_matrix.h"
#include "rotating_orb_anim.h"
#include <iostream>
#include <memory>
//these look so good man
int main() {
    // keot this here just in case
    using namespace tfw;

    std::cout << "--- LED Animation Demo ---" << std::endl;

    // 1. Initialize the LED Manager
    auto led_manager = std::make_unique<LEDManager>();
    if (!led_manager->Initialize()) {
        std::cerr << "Fatal: Could not initialize LED Manager. Exiting." << std::endl;
        return 1;
    }

    // 2. Define the animations we want to show
    Glow idle_animation(5, led_color_t{40, 120, 255}, led_color_t{5,5,10});
    
    // IDLE
    const int idle_duration_s = 15;
    std::cout << "\nPlaying IDLE animation for " << idle_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(idle_animation, idle_duration_s);

    
    // "Respond to User" - Orange Glow
    Glow respond_animation(5, {255, 140, 0}, {10, 5, 0});
    const int respond_duration_s = 15;
    std::cout << "\nPlaying RESPOND_TO_USER animation for " << respond_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(respond_animation, respond_duration_s);

    // "Error" - Red Glow
    Glow error_animation(5, {255, 0, 0}, {25, 5, 5});
    const int error_duration_s = 15;
    std::cout << "\nPlaying ERROR animation for " << error_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(error_animation, error_duration_s);

    // "Reasoning" - White orb with grey background
    RotatingOrbAnimator reasoning_animation({0.0f, 0.0f, 1.0f}, {128, 128, 128});
    const int reasoning_duration_s = 15;
    std::cout << "\nPlaying REASONING animation for " << reasoning_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(reasoning_animation, reasoning_duration_s);


    // LOADER use for booting up (models loading)
    Loader loader_animation({20, 150, 40}, 3000); // Green loader, 3 second duration (you can cchange ezpz)
    const int loader_duration_s = 15;
    std::cout << "\nPlaying LOADER animation for " << loader_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(loader_animation, loader_duration_s);


    // Connecting - Orange orb with black background
    RotatingOrbAnimator connecting_animation({30.0f, 1.0f, 1.0f}, {0, 0, 0}); // Orange orb, black background just change color for a different state
    const int connecting_duration_s = 15;
    std::cout << "\nPlaying CONNECTING animation for " << connecting_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(connecting_animation, connecting_duration_s);
    

    // 4. Clean up
    std::cout << "\nDemo finished. Cleaning up." << std::endl;
    led_manager->Clear();

    return 0;
} 