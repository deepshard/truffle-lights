#include "ledmgr.h"
#include "led_matrix.h"
#include "rotating_orb_anim.h"
#include <iostream>
#include <memory>
#include <csignal>
#include <atomic>

static std::atomic<bool> keep_running(true);

static void signal_handler(int){
    keep_running.store(false);
}

int main() {
    // keot this here just in case
    using namespace tfw;

    std::signal(SIGINT, signal_handler);

    std::cout << "--- LED Animation Demo ---" << std::endl;

    // 1. Initialize the LED Manager
    auto led_manager = std::make_unique<LEDManager>();
    if (!led_manager->Initialize()) {
        std::cerr << "Fatal: Could not initialize LED Manager. Exiting." << std::endl;
        return 1;
    }
// IDLE blue glow
    while(keep_running.load()){
        Glow idle_animation(5, led_color_t{40, 120, 255}, led_color_t{5,5,10});
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

        // "Reasoning" - White orb with grey background when models thinking (user hits enter)
        RotatingOrbAnimator reasoning_animation({0.0f, 0.0f, 1.0f}, {128, 128, 128});
        const int reasoning_duration_s = 15;
        std::cout << "\nPlaying REASONING animation for " << reasoning_duration_s << " seconds..." << std::endl;
        led_manager->PlayAnimation(reasoning_animation, reasoning_duration_s);

        // "Loader" - Green Loader
        Loader loader_animation({20, 150, 40}, 3000);
        const int loader_duration_s = 15;
        std::cout << "\nPlaying LOADER animation for " << loader_duration_s << " seconds..." << std::endl;
        led_manager->PlayAnimation(loader_animation, loader_duration_s);

        // "Connecting" - Orange orb with black background (can tune colors) wanna use for wifi/gpc connection states
        RotatingOrbAnimator connecting_animation({30.0f, 1.0f, 1.0f}, {0, 0, 0});
        const int connecting_duration_s = 15;
        std::cout << "\nPlaying CONNECTING animation for " << connecting_duration_s << " seconds..." << std::endl;
        led_manager->PlayAnimation(connecting_animation, connecting_duration_s);

        // "Typing" - Blue orb with pearly background (can tune colors and speed) wanna use for user typing, spins faster as they typew faster (use classification requests as metric?)
        RotatingOrbAnimator typing_animation({240.0f, 1.0f, 1.0f}, {200, 200, 220}, 90.0f);
        std::cout << "\nPlaying TYPING animation: 10s slow, 10s fast..." << std::endl;
        led_manager->PlayAnimation(typing_animation, 10);
        typing_animation.setRotationSpeed(300.0f);
        led_manager->PlayAnimation(typing_animation, 10);
    }

    std::cout << "\nDemo finished. Cleaning up." << std::endl;
    led_manager->Clear();

    return 0;
} 