#pragma once

#include "led_color.h"
#include "led_matrix.h"
#include "ledmgr.h" 
#include <array>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace tfw {

class LEDManager; // forward declaration

// use by two states: STARTUP and TYPING just diff colors
class RotatingOrbAnimator {
public:
    RotatingOrbAnimator(HSV orb_hsv,
                        led_color_t background = {128, 128, 128},
                        float rotation_speed_deg_per_sec = 300.0f,
                        float blur_sigma = 3.5f,
                        float intensity = 1.2f)
        : orbHSV(orb_hsv),
          bg_colour(background),
          rot_speed(rotation_speed_deg_per_sec),
          sigma(blur_sigma),
          intensity(intensity),
          angle(0.0f),
          last_update(std::chrono::high_resolution_clock::now()) {}

    // Call once per frame. Returns how long (in µs) the caller should sleep.
    uint64_t operator()(LEDManager* mgr);


    void setRotationSpeed(float deg_per_sec) { rot_speed = deg_per_sec; }
    float getRotationSpeed() const { return rot_speed; }

private:
    HSV          orbHSV;
    led_color_t  bg_colour;
    float        rot_speed;
    float        sigma;
    float        intensity;

    float angle; // current angle in degrees
    std::chrono::time_point<std::chrono::high_resolution_clock> last_update;

    // Pre-computed polar lookup table for every LED on the matrix.
    static inline const std::array<polar_t, LED_COUNT> led_lut = [](){
        std::array<polar_t, LED_COUNT> lut{};
        int idx = 0;
        for(int ring = 0; ring < 5; ++ring){
            int count = ring_sizes[ring];
            for(int i = 0; i < count; ++i){
                float theta = (ring == 0) ? 0.0f : DEG2RAD((360.0f / count) * i);
                lut[idx++] = polar_t{theta, static_cast<float>(ring)};
            }
        }
        return lut;
    }();
};

// Inline implementation
inline uint64_t RotatingOrbAnimator::operator()(LEDManager* mgr) {
    if(!mgr) return 20 * 1000;

    // Ensure matrix exists
    if(!mgr->matrix) return 20 * 1000;

    // Clear matrix for a fresh frame
    mgr->matrix->Clear(mgr->leds);

    // Time delta
    auto now = std::chrono::high_resolution_clock::now();
    float elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
    last_update = now;

    // Advance angle
    angle += (rot_speed / 1000.0f) * elapsed_ms;
    if(angle >= 360.0f) angle -= 360.0f;

    polar_t orb_position = polar_t::Degrees(angle, 3);

    // Fill background colour first
    for(int i = 0; i < LED_COUNT; ++i){
        mgr->leds[i] = bg_colour;
    }

    // Apply Gaussian-blurred orb contribution
    for(int i = 0; i < LED_COUNT; ++i){
        polar_t p = led_lut[i];
        float dtheta = angularDifference(p.theta, orb_position.theta);
        float rbar   = (p.r + orb_position.r) * 0.5f;
        float dr     = p.r - orb_position.r;
        float d2     = (dtheta * rbar) * (dtheta * rbar) + (dr * dr);
        float F      = std::exp(-d2 / (2 * sigma * sigma));

        led_color_t orb_rgb = hsv2rgb(orbHSV) * (intensity * F);
        float blend = std::min(1.0f, F * 2.0f);

        mgr->leds[i].r = static_cast<uint8_t>((1.0f - blend) * bg_colour.r + blend * orb_rgb.r);
        mgr->leds[i].g = static_cast<uint8_t>((1.0f - blend) * bg_colour.g + blend * orb_rgb.g);
        mgr->leds[i].b = static_cast<uint8_t>((1.0f - blend) * bg_colour.b + blend * orb_rgb.b);
    }

    mgr->update_leds();
    return 20 * 1000; // ~20ms frame
}

} 