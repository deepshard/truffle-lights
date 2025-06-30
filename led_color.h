#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <random> 
#include <utility>


#define LED_COUNT 61
#define WS2812B_SPI_SPEED 2500000
#define WS2812B_HIGH 0b11100000  //  WS2812 "1"
#define WS2812B_LOW  0b10000000  //  WS2812 "0"

#define DEFAULT_COLOR {128, 128, 128}


#define M_PI_F		((float)(M_PI))	
#define RAD2DEG( x )  ( (float)(x) * (float)(180.f / M_PI_F) )
#define DEG2RAD( x )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#define RAD2DEGNUM 57.295779513082f


struct led_color_t {
    uint8_t r,g,b;

    led_color_t operator+(const led_color_t& other) const {
        return { static_cast<uint8_t>(std::min(255, r + other.r)), static_cast<uint8_t>(std::min(255, g + other.g)), static_cast<uint8_t>(std::min(255, b + other.b)) };
    }
    led_color_t operator-(const led_color_t& other) const {
        return { static_cast<uint8_t>(std::max(0, r - other.r)), static_cast<uint8_t>(std::max(0, g - other.g)), static_cast<uint8_t>(std::max(0, b - other.b)) };
    }
    led_color_t operator*(float scalar) const {

        return {
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, r * scalar))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, g * scalar))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, b * scalar)))
        };
    }
    led_color_t operator/(float scalar) const {
        return {
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, r / scalar))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, g / scalar))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, b / scalar)))
        };
    }
    bool operator==(const led_color_t& other) const {
        return r == other.r && g == other.g && b == other.b;
    }

    bool operator!=(const led_color_t& other) const {
        return !(*this == other);
    }
    bool operator>(const led_color_t& other) const {
        return r > other.r && g > other.g && b > other.b;
    }
    bool operator<(const led_color_t& other) const {
        return r < other.r && g < other.g && b < other.b;
    }

    bool operator>=(const led_color_t& other) const {
        return r >= other.r && g >= other.g && b >= other.b;
    }
    bool operator<=(const led_color_t& other) const {
        return r <= other.r && g <= other.g && b <= other.b;
    }
    bool operator<(uint8_t val) const {
        return r < val && g < val && b < val;
    }
    bool operator>(uint8_t val) const {
        return r > val && g > val && b > val;
    }
    bool operator==(uint8_t val) const {
        return r == val && g == val && b == val;
    }
    
    operator bool() const {
        return r || g || b;
    }
};

inline void encode_color(uint8_t r, uint8_t g, uint8_t b, char* buffer) {
    for (int i = 0; i < 8; i++) {
        buffer[i] = (g & (1 << (7 - i))) ? WS2812B_HIGH : WS2812B_LOW;
        buffer[8 + i] = (r & (1 << (7 - i))) ? WS2812B_HIGH : WS2812B_LOW;
        buffer[16 + i] = (b & (1 << (7 - i))) ? WS2812B_HIGH : WS2812B_LOW;
    }
}
inline void encode_color(const led_color_t& c, char* buffer) {
    for (int i = 0; i < 8; i++) {
        buffer[i] = (c.g & (1 << (7 - i))) ? WS2812B_HIGH : WS2812B_LOW;
        buffer[8 + i] = (c.r & (1 << (7 - i))) ? WS2812B_HIGH : WS2812B_LOW;
        buffer[16 + i] = (c.b & (1 << (7 - i))) ? WS2812B_HIGH : WS2812B_LOW;
    }
}


struct HSV {
    float h; // [0,360)
    float s; // [0,1]
    float v; // [0,1]
};

inline led_color_t hsv2rgb(const HSV& hsv) {
    float H = hsv.h;
    float S = hsv.s;
    float V = hsv.v;
    float C = V * S;
    float X = C * (1 - std::fabs(fmod(H/60.0f, 2) - 1));
    float m = V - C;

    float r1, g1, b1;
    if      (H <  60) { r1 = C; g1 = X; b1 = 0; }
    else if (H < 120) { r1 = X; g1 = C; b1 = 0; }
    else if (H < 180) { r1 = 0; g1 = C; b1 = X; }
    else if (H < 240) { r1 = 0; g1 = X; b1 = C; }
    else if (H < 300) { r1 = X; g1 = 0; b1 = C; }
    else              { r1 = C; g1 = 0; b1 = X; }

    uint8_t R = static_cast<uint8_t>(std::round((r1 + m) * 255));
    uint8_t G = static_cast<uint8_t>(std::round((g1 + m) * 255));
    uint8_t B = static_cast<uint8_t>(std::round((b1 + m) * 255));
    return { R, G, B };
}


inline uint8_t generate_random_uint8() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint8_t> dis(0, 255);
    return dis(gen);
}

inline led_color_t generate_random_color(int min_value = 50) {
    uint8_t r = generate_random_uint8();
    uint8_t g = generate_random_uint8();
    uint8_t b = generate_random_uint8();

    int avg = (r + g + b) / 3;
    if(avg < min_value) {
        return generate_random_color();
    }
    return {r,g,b};
}


inline float easeInOut(float t) {
    return 0.5f * (1.0f - cosf(M_PI_F * t));
}
// linear interpolate
inline float mixf(float a, float b, float t) {
    return a + (b - a) * t;
}
