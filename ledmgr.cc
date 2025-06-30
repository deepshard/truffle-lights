#include "ledmgr.h"
#include "spi.h"
#include "led_color.h"
#include "led_matrix.h"
#include "rotating_orb_anim.h" // Included for friend class access

#include <vector>
#include <array>
#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>


static_assert(LED_COUNT * 24 < SPI_BUFFER_SIZE );

namespace tfw {

LEDManager::LEDManager() : functional(false) {
    // Constructor is now much simpler.
}

LEDManager::~LEDManager() {
    if (functional) {
        Clear();
    }
    std::cout << "[LEDManager] Shutting down." << std::endl;
}

bool LEDManager::Initialize() {
    std::cout << "[LEDManager] Initializing..." << std::endl;
    spi = std::make_unique<spi_t>(WS2812B_SPI_SPEED);
    if (spi->state != SPI_OPEN) {
        std::cerr << "[LEDManager] Error: Failed to initialize SPI." << std::endl;
        functional = false;
        return false;
    }
    
    matrix = std::make_unique<LEDMatrix>();
    functional = true;
    Clear();
    std::cout << "[LEDManager] Initialization successful." << std::endl;
    return true;
}

void LEDManager::Clear() {
    if (!functional) return;
    matrix->Clear(leds);
    update_leds();
}

void LEDManager::PlayAnimation(Animatable& animation, int duration_seconds) {
    if (!functional) {
        std::cerr << "[LEDManager] Error: Cannot play animation, not initialized." << std::endl;
        return;
    }

    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(duration_seconds);

    while (std::chrono::steady_clock::now() - start_time < duration) {
        matrix->Clear(leds);
        
        animation.Update();
        animation.Draw(matrix.get());
        
        matrix->Update(leds);
        update_leds();
        
        // Sleep for ~20ms to achieve a smooth ~50 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void LEDManager::PlayAnimation(RotatingOrbAnimator& animation, int duration_seconds) {
    if (!functional) {
        std::cerr << "[LEDManager] Error: Cannot play animation, not initialized." << std::endl;
        return;
    }

    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(duration_seconds);

    while (std::chrono::steady_clock::now() - start_time < duration) {
        // RotatingOrbAnimator uses a different interface - it returns the sleep time
        uint64_t sleep_micros = animation(this);
        
        // Convert microseconds to milliseconds and sleep
        if (sleep_micros > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_micros));
        }
    }
}

void LEDManager::update_leds() {
    if (!functional) return;
    char tx[LED_COUNT * 24] = {0};
    for (int j = 0; j < LED_COUNT; j++) {
        encode_color(this->leds[j], &tx[j * 24]);
    }
    if (!spi->transfer(tx, sizeof(tx))) {
        std::cerr << "[LEDManager] SPI transfer failed." << std::endl;
    }
    usleep(5);
}

} // namespace tfw

