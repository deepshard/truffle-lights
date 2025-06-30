#include "ledmgr.h"
#include "led_matrix.h"
#include <iostream>
#include <memory>

int main() {
    // The main namespace for the LED components
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
    
    // 3. Play the idle animation
    const int idle_duration_s = 15;
    std::cout << "\nPlaying IDLE animation for " << idle_duration_s << " seconds..." << std::endl;
    led_manager->PlayAnimation(idle_animation, idle_duration_s);

    // --- You can add more animations to the sequence here ---
    // Example:
    // Loader loading_animation({20, 150, 40}, 2000);
    // std::cout << "\nPlaying LOADER animation for 2 seconds..." << std::endl;
    // led_manager->PlayAnimation(loading_animation, 2);


    // 4. Clean up
    std::cout << "\nDemo finished. Cleaning up." << std::endl;
    led_manager->Clear();

    return 0;
} 