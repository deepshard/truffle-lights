#pragma once

#include "led_color.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>

// Forward declarations
class spi_t;
class LEDMatrix;
class Animatable;

namespace tfw {

class LEDManager {
public:
    explicit LEDManager();
    ~LEDManager();
    
    // Initializes the SPI hardware. Returns false on failure.
    bool Initialize();

    // Plays a given animation for a specified duration.
    void PlayAnimation(Animatable& animation, int duration_seconds);

    // Turns all LEDs off.
    void Clear();

private:
    friend class RotatingOrbAnimator; // Animator needs access to update_leds
    void update_leds();

    std::unique_ptr<spi_t> spi;
    bool functional = false;
    
    std::array<led_color_t, LED_COUNT> leds;
    std::unique_ptr<LEDMatrix> matrix;
};

} 