#pragma once
#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "led_color.h"



using LEDArray = std::array<led_color_t, LED_COUNT>;

//1, 8, 12, 16, 24
static constexpr const int ring_sizes[5] = {1, 8, 12, 16, 24};
static constexpr const float ring_incs[5] = {0.f, 45.f, 30.f, 22.5f, 15.f};


inline float ringunit(int ring, float mul){
    return ((ring_incs[ring] - 0.6) * mul);
}

struct polar_t{
    float theta;
    float r;    // now float so we can interpolate smoothly
    
    static polar_t Degrees(float angle_deg, int radius){
        return {DEG2RAD(angle_deg), static_cast<float>(radius)};
    }
    void rotate_deg(float deg){
        theta += DEG2RAD(deg);
        normalize();
    }
    float angle_deg() const {
        return RAD2DEG(theta);
    }
    void set_angle_deg(float angle_deg){
        theta = DEG2RAD(angle_deg);
    }
    polar_t& normalize(){
        while(theta >= 2.f * M_PI_F) theta -= 2.f * M_PI_F;
        while(theta < 0.f) theta += 2.f * M_PI_F;
        r = fabsf(r);
        if(r > 4.0f) r = 4.0f;
        return *this;
    }
    operator std::pair<float, int>() const {
        return {angle_deg(), static_cast<int>(r)};
    }
    polar_t operator+ (const polar_t& other) const {
        return {theta + other.theta, r + other.r};
    }
    polar_t operator- (const polar_t& other) const {
        return {theta - other.theta, r - other.r};
    }
    polar_t operator* (float scalar) const {
        return {theta * scalar, r * scalar};
    }
    polar_t operator/ (float scalar) const {
        return {theta / scalar, r / scalar};
    }

    bool operator==(const polar_t& other) const {
        //use epsilon!
        const float eps = 0.01f;
        return std::abs(theta - other.theta) < eps && std::abs(r - other.r) < eps;
    }
};

class LEDRingBase{
    public:
        LEDRingBase(int index) : index(index) {
        }
    
        virtual void Update(LEDArray& leds) = 0;
    
        virtual const int Count() const = 0;
        const auto Index() const {
            return index;
        }
        virtual void clear(led_color_t clr = {0,0,0}) = 0;
        virtual void set_led(int idx, led_color_t color) = 0;
    protected:
        const int index;
    };
    
    template <std::size_t N>
    class LEDRing : public LEDRingBase {
    public:
        LEDRing(int index) : LEDRingBase(index), led_count(leds.size()) {
            start_idx = 0;
            for(int i = 0; i < index; ++i)
                start_idx += ring_sizes[i];
            end_idx = start_idx + N;
            assert(end_idx <= LED_COUNT);
    
            //flip indices 
            int temp = start_idx;
            start_idx = LED_COUNT - end_idx;
            end_idx = LED_COUNT - temp;
            printf("Ring[%d] %d: %d - %d\n", index, led_count, start_idx, end_idx);
            
            for(auto& led : leds)
                led = {125,125,125};// generate_random_color();
        }
    
        const int Count() const override { return led_count; }
    
    
        void Update(LEDArray& all) override {
            for(int i = start_idx; i < end_idx; ++i){
                if(index == 4) all[i] = leds[i - start_idx] * 0.37f;
                else if(index == 3) all[i] = leds[i - start_idx] * 1.66f;
                else all[i] = leds[i - start_idx];
            }
    
        }
    
    
        void set_led(int idx, led_color_t color) override {
            if(idx < 0 || idx >= N) throw std::out_of_range("LED index out of range");
            if(color)
                leds[idx] = leds[idx] + color;
            else 
                leds[idx] = color;
        }
    
        void clear(led_color_t clr = {0,0,0}) override {
            for(auto& led : leds)
                led = clr;
        }
    protected:
        const int led_count;
        int start_idx;
        int end_idx;
        std::array<led_color_t, N> leds; 
    };
    
    class LEDMatrix {
    public:
        LEDMatrix() {
            this->rings = {
                std::make_unique<LEDRing<1>>(0),
                std::make_unique<LEDRing<8>>(1),
                std::make_unique<LEDRing<12>>(2),
                std::make_unique<LEDRing<16>>(3),
                std::make_unique<LEDRing<24>>(4),
            };
            set_all({0,0,0});
        }
    
        void Clear(LEDArray& leds, led_color_t clr = {0,0,0}){
            for(auto& ring : rings){
                ring->clear(clr);
            }
            this->Update(leds);
        }
        //returns ring index, led index within ring 
        std::pair<int, int> polar_to_ring(float angle_deg, int radius){
            if(std::abs(radius) > 4) return {0xffff, 0xffff};
            if(radius == 0) return {0, 0};
            if(angle_deg >= 360.f){
                while(angle_deg >= 360.f) angle_deg -= 360.f;
            }
            if(angle_deg < 0.f){
                while(angle_deg < 0.f) angle_deg += 360.f;
            }
    
            int ring = abs(radius); 
            float angle = DEG2RAD(angle_deg);
            float led_idx_f = (angle / (2.f * M_PI_F) ) * (ring_sizes[ring]);
           
            int led = static_cast<int>(std::round(led_idx_f));
            if(led != 0 && led == ring_sizes[ring]) led = 0;
    
          //  printf("angle: %f, radius: %d, ring: %d, led: %d\n", angle_deg, radius, ring, led); 
            return {ring, led};
        }
        std::pair<int, int> polar_to_ring(polar_t coords){
            if(std::abs(coords.r) > 4.0f) return {0xffff, 0xffff};
            if(coords.r < 0.5f) return {0, 0};  // Consider values less than 0.5 as center
            coords.normalize();
    
            int ring = static_cast<int>(std::round(coords.r)); 
            ring = std::min(4, std::max(0, ring)); // Clamp to valid range
           
            float led_idx_f = (coords.theta / (2.f * M_PI_F) ) * ( static_cast<float>(ring_sizes[ring]));
           
            int led = static_cast<int>(std::round(led_idx_f) );
            if(led != 0 && led == ring_sizes[ring]) led = 0;
    
            //printf("angle: %f, radius: %f, ring: %d, led: %d\n", RAD2DEG(coords.theta), coords.r, ring, led); 
            return {ring, led};
        }
        std::pair<int, int> grid_to_ring(int x, int y) {
            if(x == 0 && y == 0) return {0, 0};
            if(std::abs(x) > 4 || std::abs(y) > 4) return {0xffff, 0xffff};
    
            float theta = atan2(y, x);
    
            int radius = static_cast<int>(std::round(std::sqrt(x * x + y * y)));
    
          
            return polar_to_ring(RAD2DEG(theta), radius);
        }
    
        void Update(LEDArray& leds) {
    
            for(auto& ring : rings){
                ring->Update(leds);
            }
    
    
        }
    
    
        void set_led(float angle_deg, int radius, led_color_t color){
            auto [ring, led] = polar_to_ring(angle_deg, radius);
            if(ring == 0xffff || led == 0xffff) {
                throw std::out_of_range("Invalid coords: " + std::to_string(angle_deg) + ", " + std::to_string(radius));
            }
            rings[ring]->set_led(led, color);
        }
    
        void set_led(float angle_deg, float radius, led_color_t color){
            polar_t coords = {DEG2RAD(angle_deg), radius};
            set_led(coords, color);
        }
    
        void set_led(polar_t coords, led_color_t color){
            // Ensure coords are normalized before processing
            coords.normalize();
            auto [ring, led] = polar_to_ring(coords);
            if(ring == 0xffff || led == 0xffff){
                printf("Invalid coords: %f, %f\n", RAD2DEG(coords.theta), coords.r);
                return;
            }
            rings[ring]->set_led(led, color);
        }
    
        void set_all(led_color_t color){
            // Add debug print to verify this is actually called
            static int set_all_count = 0;
            if (set_all_count++ % 10 == 0) {
                printf("DEBUG: LEDMatrix::set_all called %d times with color {%d,%d,%d}\n", 
                      set_all_count, color.r, color.g, color.b);
            }
            
            for(auto& ring : rings){
                for(int i = 0; i < ring->Count(); ++i){
                    ring->set_led(i, color);
                }
            }
        }
    
    protected:
        std::array<std::unique_ptr<LEDRingBase>, 5> rings;
    };
    
    struct animLED{
        polar_t origin;
        led_color_t color;
    };
    
    class Animatable{
    public:
        Animatable() {
            start = std::chrono::high_resolution_clock::now();
        }
        virtual void Update() = 0;
        virtual void Draw(LEDMatrix* matrix) = 0;
        virtual ~Animatable() = default;
    
        void SetOrigin(float angle_deg, int radius){
           this->origin = polar_t::Degrees(angle_deg, static_cast<float>(radius));
        }
        
        // new overload to set a full polar directly
        void SetOrigin(const polar_t &p){
            origin = p;
        }
        
        polar_t GetOrigin() const {
            return origin;
        }
        
        // Make start time accessible for fusion calculations
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        
    protected:
        std::vector<animLED> leds;
        polar_t origin;
    };
    
    class Orb : public Animatable{
        public:
        Orb(int size = 3,  led_color_t base_color = {245,245,245}, polar_t origin = {0.f, 3.0f}) {
            // Make sure the origin is normalized
            this->origin = origin.normalize();
            this->color = base_color;
            float mul = 1.f;
            const float mulmul = 0.85f;
            //ORIGIN
            leds.push_back({{DEG2RAD(0.f), 0.0f}, base_color});
            //LINE OF 3 DOWN CENTER
            leds.push_back({{DEG2RAD(0.f), -1.0f}, base_color});
            leds.push_back({{DEG2RAD(0.f), 1.0f}, base_color});
            if(origin.r <= 2.0f)
                leds.push_back({{DEG2RAD(0.f), 2.0f}, base_color});
            if(size == 1) return;
    
    
            //LEFT AND RIGHT OF CENTER
            leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r), -1.f)), 0.0f}, base_color});
            leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r), 1.f)), 0.0f}, base_color});   
    
    
            //LEFT AND RIGHT TOP
            leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r) + 1, -1.f)), 1.0f}, base_color});
            leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r) + 1, 1.f)), 1.0f}, base_color});   
            if(origin.r <= 2.0f){
                leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r) + 2, -1.f)), 2.0f}, base_color});
                leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r) + 2, 1.f)), 2.0f}, base_color});   
            }
            //LEFT AND RIGHT BOTTOM
            leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r) - 1, -1.f)), -1.0f}, base_color});
            leds.push_back({{DEG2RAD(ringunit(static_cast<int>(origin.r) + 1, 1.f)), -1.0f}, base_color});   
            if(size == 2) return;
    
            
            for(int i = 3; i <= size; ++i){
                int ii = i - 2;
                for(int j = -1; j <= 1; ++j){
                    float angle = ringunit(static_cast<int>(origin.r) + j, ii * -1.f); // -11.5f * ii;
                   // if(j == 1) angle = -4.f * ii;
                    leds.push_back({{DEG2RAD(angle), static_cast<float>(j)}, base_color * mul});
                   // printf("Orb[%d]: %f, %d\n", i, -20.f * ii, j);
                }
                for(int j = -1; j <= 1; ++j){
                    float angle = ringunit(static_cast<int>(origin.r) + j, ii * 1.f); // -11.5f * ii;
                  //  float angle = 11.f * ii;
                    //if(j == 1) angle = 8.f * ii;
                    leds.push_back({{DEG2RAD(angle), static_cast<float>(j)}, base_color * mul});
                //     printf("Orb[%d]: %f, %d\n", i, 25.f * ii, j);
                }
                mul *= mulmul;
            }
            last_update = std::chrono::high_resolution_clock::now();
        }
    
        void Update() override {
            uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - last_update).count();
            if(us < 2000) return; //smoothing this would be nice
            last_update = std::chrono::high_resolution_clock::now();
    
           uint64_t delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
            
            
            float scale = exp((rot_speed * 0.0001f) / max_speed);
            const float mul = 0.99f * scale;
            const float imul = (1.f / mul) * scale;
    
            const float base_speed = 1.001f;
     
            const uint64_t hold_time = 2500;
            if(!speed_up && delta_ms > (hold_time + last_speedchange) && std::abs(rot_speed) > base_speed){
                  
                rot_speed *= (imul);
                //printf("not speed up: %f \n", rot_speed);
            }
            else if(speed_up && delta_ms > (hold_time + last_speedchange) && std::abs(rot_speed) < max_speed){
              
                rot_speed *= (mul);
                //printf("speed up %f \n", rot_speed);
            }
            if((rot_speed <= base_speed || std::abs(rot_speed) > max_speed) && delta_ms > (hold_time + last_speedchange)){
                rot_speed = (speed_up) ? (base_speed + 0.01f) : (max_speed - 0.01f);
                last_speedchange = delta_ms;
                speed_up = !speed_up;
                if(speed_up) m *= -1.f;
               // printf("speed change: %f, %d, %lu\n", rot_speed, speed_up, last_speedchange);
            }
    
           this->origin.rotate_deg(rot_speed * m);
           
           // Update all LED colors if the main color has changed
           if (color != prev_color) {
               UpdateLEDColors();
               prev_color = color;
           }
        }
        
        // Update all LED colors based on the current color
        void UpdateLEDColors() {
            // Keep the original pattern but update with new color
            float mul = 1.0f;
            const float mulmul = 0.85f;
            
            // Update center LEDs with the main color
            for (size_t i = 0; i < std::min((size_t)10, leds.size()); i++) {
                leds[i].color = color;
            }
            
            // Update outer LEDs with diminishing intensity
            for (size_t i = 10; i < leds.size(); i++) {
                int ring = (i - 10) / 6 + 3;  // Calculate the effective "ring" based on LED index
                int position = (i - 10) % 6;  // Position within the ring
                
                // Apply intensity falloff based on ring position
                float intensity = 1.0f;
                for (int r = 3; r <= ring; r++) {
                    intensity *= mulmul;
                }
                
                leds[i].color = color * intensity;
            }
        }
        
        void Draw(LEDMatrix* matrix) override {
            for(auto& led : leds){
                auto real_pos = origin + led.origin;
                matrix->set_led(real_pos, led.color);
            }
        }
        
        // Set the orb's color and update all LEDs
        void SetColor(led_color_t new_color) {
            color = new_color;
            UpdateLEDColors();
        }
        
        // Public color property that can be directly modified by TransitionSpiral
        led_color_t color;
        led_color_t prev_color = {0,0,0};  // Track previous color to detect changes
        
        float max_speed = 270.f;
        float rot_speed = max_speed;
        bool speed_up = false;
        uint64_t last_speedchange = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
        float m = 1.f;
    };
    
    
    
    // was old glow version, this one doesnt flash still needs morepolish
    //but yea used by rtu, error and idle
    class Glow : public Animatable{
        public:
            Glow(int size = 3, led_color_t base_color = {255,255,255}, led_color_t min_color = {0,0,0}) 
            : base_color(base_color), min_color(min_color), max_size(size) {
            if(size < 3) throw std::invalid_argument("Glow size must be at least 3");
            this->SetOrigin(0.f, 0);
            leds.push_back({{DEG2RAD(0.f), 0.0f}, {0,0,0}});

            for(int i = 0; i < size; ++i){
                for(int j = 0; j < ring_sizes[i]; ++j){
                    leds.push_back({{DEG2RAD(ringunit(i, j)), static_cast<float>(i)}, {0,0,0}});
                }
            }
            current_size = 0.f;
            inc = 0.015f;
            last_update = std::chrono::high_resolution_clock::now();
        }
        void set_ring(int ring, led_color_t color){
            int idx = 1;
             for(int i = 0; i < ring; ++i){
                idx += ring_sizes[i];
            }
            for(int i = 0; i < ring_sizes[ring]; ++i){
                leds[idx + i].color = color;
            }
        }
        void Update() override {
            for(auto& led : leds){
                led.color = min_color;
            }
            
            uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - last_update).count();
            if(us < 800) return; // smoothing
            last_update = std::chrono::high_resolution_clock::now();
            
            // Constant dim core intensity to ensure center never goes completely off
            const float dim_core_intensity = 0.25f;
            
            // CRITICAL: Set center ring first to ensure it's always on regardless of animation phase
            set_ring(0, min_color + ((base_color - min_color) * dim_core_intensity));
            
            // For each ring, calculate its brightness based on a single, symmetrical pulse
            const float pulse_width = 2.5f; // Defines the softness of the pulse edge
            for (int ring = 1; ring < max_size; ring++) {
                float intensity = 0.0f;

                // Calculate the distance of the ring from the current center of the pulse
                const float dist = fabsf(current_size - static_cast<float>(ring));

                // The pulse has a soft edge. We only calculate intensity for rings inside the pulse width.
                if (dist < pulse_width) {
                    // Normalize distance to a 0.0-1.0 value.
                    // 1.0 = at the center of the pulse. 0.0 = at the very edge.
                    const float t = 1.0f - (dist / pulse_width);
                    // Use a smoothstep function for a soft, organic falloff from the pulse center.
                    intensity = t * t * (3.0f - 2.0f * t);
                }
                
                // Apply a natural falloff from the matrix center to make the pulse dimmer as it moves outward.
                const float distance_falloff = 1.0f - (static_cast<float>(ring) * 0.1f);
                intensity *= std::max(0.0f, distance_falloff);
                
                // Apply final intensity to the color
                const led_color_t ring_color = min_color + ((base_color - min_color) * intensity);
                set_ring(ring, ring_color);
            }
            
            // Update animation timing
            current_size += inc;
            if(current_size >= static_cast<float>(max_size)) { 
                inc = inc * -1.f; 
                current_size = static_cast<float>(max_size) - 0.01f; 
                pulses++; 
            }
            if(current_size <= 0.f) { 
                inc = inc * -1.f; 
                current_size = 0.001f; 
                pulses++; 
            }
        }

        void Draw(LEDMatrix* matrix) override {
            for(auto& led : leds){
                auto real_pos = origin + led.origin;
                matrix->set_led(real_pos, led.color);
            }
        }

        led_color_t base_color;
        led_color_t min_color;
        int max_size;
        float current_size;
        float inc;
        int pulses = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
    };
    
    class Loader : public Animatable {
    public:
        Loader(led_color_t color = {20, 150, 40}, uint32_t duration_ms = 1500) 
        : color(color), 
          current_ring(0), 
          current_led_on_ring(0),
          is_finished(false) {
            // Calculate delay between steps to match the total duration
            animation_delay_us = (static_cast<uint64_t>(duration_ms) * 1000) / LED_COUNT;
            last_update = std::chrono::high_resolution_clock::now();
        }

        bool finished() const {
            return is_finished;
        }

        void Update() override {
            if (is_finished) return;

            uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - last_update).count();
            if (us < animation_delay_us) return;
            last_update = std::chrono::high_resolution_clock::now();

            // Advance to the next LED
            current_led_on_ring++;

            // If we've finished the current ring, move to the next one
            if (current_led_on_ring > ring_sizes[current_ring]) {
                current_ring++;
                current_led_on_ring = 1; // Start from the first LED of the new ring
            }
            

            if (current_ring >= 5) {
                is_finished = true;
            }
        }

        void Draw(LEDMatrix* matrix) override {
            // Draw all completed rings
            for (int r = 0; r < current_ring; ++r) {
                const int num_leds = ring_sizes[r];
                // Ring 0  the center
                if (r == 0) {
                    matrix->set_led(0.0f, 0, color);
                    continue;
                }
                const float angle_step = 360.0f / static_cast<float>(num_leds);
                for (int l = 0; l < num_leds; ++l) {
                    matrix->set_led(l * angle_step, r, color);
                }
            }

            // Draw the LEDs on the current, in-progress ring
            if (!is_finished && current_ring < 5) {
                const int num_leds = ring_sizes[current_ring];
                if (current_ring == 0 && current_led_on_ring > 0) {
                     matrix->set_led(0.0f, 0, color);
                } else {
                    const float angle_step = 360.0f / static_cast<float>(num_leds);
                    for (int l = 0; l < current_led_on_ring; ++l) {
                        matrix->set_led(l * angle_step, current_ring, color);
                    }
                }
            }
        }

    private:
        led_color_t color;
        uint64_t animation_delay_us;
        int current_ring;
        int current_led_on_ring;
        bool is_finished;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
    };
    
    //helper for ang diff
    static float angularDifference(float a, float b) {
        // Normalize angles to [0, 2π) if not already
        while (a < 0) a += 2.0f * M_PI_F;
        while (a >= 2.0f * M_PI_F) a -= 2.0f * M_PI_F;
        while (b < 0) b += 2.0f * M_PI_F;
        while (b >= 2.0f * M_PI_F) b -= 2.0f * M_PI_F;
        
        float d = fabs(a - b);
        d = fmod(d, 2.0f * M_PI_F);
        return (d > M_PI_F) ? (2.0f * M_PI_F - d) : d;
    }
    
    // Improved TransitionSpiral class with Gaussian blending
    class TransitionSpiral : public Animatable {
    public:
        enum Phase { IN = 0, FUSION = 1, FLASH = 2, EXPANSION = 3, OUT = 4, DONE = 5 };
        
        // Duration constants for phases
        static constexpr float T_in = 0.8f;         // Initial rotation phase
        static constexpr float T_fusion = 0.5f;     // Fusion/spiral in phase 
        static constexpr float T_flash = 0.2f;      // Flash at center
        static constexpr float T_expansion = 0.5f;  // Expansion from center
        static constexpr float T_out = 0.8f;        // Final rotation phase
        
        // Improved HSV interpolation function
        static HSV interpolateHSV(const HSV& a, const HSV& b, float t) {
            // Handle hue specially to find shortest path around the circle
            float h_diff = b.h - a.h;
            
            // Adjust for wrap-around
            if (h_diff > 180.0f) h_diff -= 360.0f;
            else if (h_diff < -180.0f) h_diff += 360.0f;
            
            float h = a.h + (h_diff * t);
            // Normalize h to [0, 360)
            while (h >= 360.0f) h -= 360.0f;
            while (h < 0.0f) h += 360.0f;
            
            return HSV{
                h,
                a.s + (b.s - a.s) * t,
                a.v + (b.v - a.v) * t
            };
        }
        
        // Constructor with HSV palettes
        TransitionSpiral(const std::array<HSV,3>& from,
                         const std::array<HSV,3>& to,
                         float duration = 1.8f)
        : hsv_from(from), hsv_to(to), phase(IN), t_phase(0.0f) {
            last_update = std::chrono::high_resolution_clock::now();
            
            // Set unique rotation speeds for each orb for more dynamic movement
            orb_speeds = {320.0f, 340.0f, 300.0f};
            
            // Set Gaussian parameters for each orb
            sigma = {1.0f, 1.0f, 1.0f};       // Gaussian blur radius
            intensity = {0.9f, 0.9f, 0.9f};   // Intensity - higher than in active state
            
            // Initialize orbs at 120 degrees apart at radius 3 (outer ring)
            for(int k = 0; k < 3; ++k) {
                // Start at radius 3 (standard outer orbit radius) for consistency
                orbs.emplace_back(std::make_unique<Orb>(4, hsv2rgb(hsv_from[k]), polar_t::Degrees(k * 120.f, 3)));
                
                // Customize each orb's rotation speed slightly for variation
                Orb* orbPtr = dynamic_cast<Orb*>(orbs[k].get());
                orbPtr->rot_speed = orb_speeds[k];
                orbPtr->max_speed = orb_speeds[k] * 1.2f;
            }
            
            // Set phase start times for timing progress
            phase_start_time = std::chrono::high_resolution_clock::now();
            this->dt = 0.0f;
            
            // Record initial positions for animation
            for (int i = 0; i < 3; i++) {
                initial_positions.push_back(dynamic_cast<Orb*>(orbs[i].get())->GetOrigin());
            }
        }
    
        // Add a method to get the current phase for debugging
        int getPhase() const { return static_cast<int>(phase); }
        
        // Add a method to get the normalized time within current phase
        float getNormalizedTime() const { 
            switch (phase) {
                case IN: return t_phase / T_in;
                case FUSION: return t_phase / T_fusion;
                case FLASH: return t_phase / T_flash;
                case EXPANSION: return t_phase / T_expansion;
                case OUT: return t_phase / T_out;
                default: return 1.0f;
            }
        }
    
        bool finished() const { return phase == DONE; }
        
        void Update() override {
            // advance t_phase based on elapsed time
            auto now = std::chrono::high_resolution_clock::now();
            dt = std::chrono::duration<float>(now - last_update).count();
            last_update = now;
            t_phase += dt;
            
            // Update phase based on timing
            bool phase_changed = false;
            switch(phase) {
                case IN:
                    if (t_phase >= T_in) { 
                        phase = FUSION; 
                        t_phase = 0.0f; 
                        phase_changed = true;
                    }
                    break;
                case FUSION:
                    if (t_phase >= T_fusion) { 
                        phase = FLASH; 
                        t_phase = 0.0f; 
                        phase_changed = true;
                    }
                    break;
                case FLASH:
                    if (t_phase >= T_flash) { 
                        phase = EXPANSION; 
                        t_phase = 0.0f; 
                        phase_changed = true;
                    }
                    break;
                case EXPANSION:
                    if (t_phase >= T_expansion) { 
                        phase = OUT; 
                        t_phase = 0.0f; 
                        phase_changed = true;
                    }
                    break;
                case OUT:
                    if (t_phase >= T_out) { 
                        phase = DONE; 
                        phase_changed = true;
                    }
                    break;
                default:
                    break;
            }
            
            if (phase_changed) {
                phase_start_time = std::chrono::high_resolution_clock::now();
            }
    
            // Calculate overall transition progress (0.0 - 1.0)
            float totalDuration = T_in + T_fusion + T_flash + T_expansion + T_out;
            float elapsedTime = std::chrono::duration<float>(now - start).count();
            float overallProgress = std::min(1.0f, elapsedTime / totalDuration);
            
            // Get normalized time within the current phase (0-1)
            float t_norm = getNormalizedTime();
            
            // Apply easing for smoother animation
            float e = easeInOut(t_norm);
            
            // Calculate distance between orbs for fusion effect
            bool close_enough_for_fusion = false;
            if (phase >= FUSION) {
                close_enough_for_fusion = true;
            } else if (phase == IN && t_norm > 0.7f) {
                // During late IN phase, check if orbs are close enough
                auto c0 = dynamic_cast<Orb*>(orbs[0].get())->GetOrigin();
                auto c1 = dynamic_cast<Orb*>(orbs[1].get())->GetOrigin();
                auto c2 = dynamic_cast<Orb*>(orbs[2].get())->GetOrigin();
                
                auto sep = [](const polar_t &a, const polar_t &b){
                    // Euclid dist in LED units
                    float dθ = angularDifference(a.theta, b.theta);
                    float r̄  = (a.r + b.r) * 0.5f;
                    float Δr  = a.r - b.r;
                    return sqrtf((dθ*r̄)*(dθ*r̄) + Δr*Δr);
                };
                
                float sep01 = sep(c0, c1);
                float sep12 = sep(c1, c2);
                float sep20 = sep(c2, c0);
                
                close_enough_for_fusion = (sep01 < 0.5f && sep12 < 0.5f && sep20 < 0.5f);
            }
    
            // Update each orb
            for(size_t k = 0; k < orbs.size(); ++k) {
                Orb* orb = dynamic_cast<Orb*>(orbs[k].get());
                
                // Base positioning based on phase
                float start_angle = initial_positions[k].theta;
                float current_radius = 3.0f;  // Default outer ring radius
                float target_angle = start_angle;
                float color_blend = 0.0f;     // How much to blend from source to target color
                
                if (phase == IN) {
                    // During IN phase: initial rotation where orbs move toward each other
                    // Create a dynamic rotational movement
                    float base_speed = orb_speeds[k] * 0.5f;
                    float angle_offset = base_speed * t_norm;
                    
                    // Orbs start moving toward meeting point (gradually slowing down)
                    float meetingPoint = 0.0f;  // Center angle for all orbs to approach
                    float approach_factor = std::pow(t_norm, 2);  // Accelerates toward end
                    
                    // Blend from initial angle toward meeting point
                    target_angle = start_angle + angle_offset * (1.0f - approach_factor);
                    target_angle += (meetingPoint - start_angle) * approach_factor;
                    
                    // Keep radius at 3.0 during most of IN phase
                    current_radius = 3.0f;
                    
                    // Begin color transition during IN phase (first 30%)
                    color_blend = 0.3f * t_norm;
                } 
                else if (phase == FUSION) {
                    // During FUSION: orbs spiral inward to center
                    float spiral_factor = e;  // Ease-in-out for smoother spiral
                    
                    // Gradually reduce radius from 3.0 to 0.5
                    current_radius = mixf(3.0f, 0.5f, spiral_factor);
                    
                    // Increase rotation as orbs get closer to center
                    float rotation_speed = mixf(1.0f, 3.0f, spiral_factor);
                    target_angle = start_angle + rotation_speed * 120.0f * spiral_factor;
                    
                    // Continue color transition (30% to 60%)
                    color_blend = 0.3f + (0.3f * e);
                }
                else if (phase == FLASH) {
                    // During FLASH phase: orbs are at center, pulsing
                    current_radius = 0.2f;
                    
                    // Slight rotation at center
                    target_angle = start_angle + t_norm * 30.0f;
                    
                    // Mid-point of color transition (60%)
                    color_blend = 0.6f;
                }
                else if (phase == EXPANSION) {
                    // During EXPANSION: orbs spiral outward from center
                    float expansion_factor = e;
                    
                    // Gradually increase radius from 0.5 to 3.0
                    current_radius = mixf(0.5f, 3.0f, expansion_factor);
                    
                    // Rotation decreases as orbs move outward
                    float rotation_speed = mixf(3.0f, 1.0f, expansion_factor);
                    target_angle = start_angle + 150.0f + rotation_speed * 120.0f * (1.0f - expansion_factor);
                    
                    // Continue color transition (60% to 90%)
                    color_blend = 0.6f + (0.3f * e);
                }
                else if (phase == OUT) {
                    // During OUT phase: orbs continue rotation at their final positions
                    // Create a dynamic rotational movement
                    float base_speed = orb_speeds[k] * 0.5f;
                    float angle_offset = base_speed * t_norm;
                    
                    // Target positions for each orb (120 degrees apart)
                    float targetAngle = k * 120.0f;
                    float approach_factor = std::pow(t_norm, 2);  // Accelerates toward end
                    
                    // Blend from current angle toward final position
                    target_angle = (start_angle + 270.0f) + angle_offset * (1.0f - approach_factor);
                    target_angle += (targetAngle - (start_angle + 270.0f)) * approach_factor;
                    
                    // Keep radius at 3.0 during OUT phase
                    current_radius = 3.0f;
                    
                    // Complete color transition (90% to 100%)
                    color_blend = 0.9f + (0.1f * t_norm);
                }
                
                // Normalize angle to valid range
                while (target_angle >= 360.0f) target_angle -= 360.0f;
                while (target_angle < 0.0f) target_angle += 360.0f;
                
                // Special case for FUSION phase: check if orbs should spiral inward
                if (phase == IN && close_enough_for_fusion && t_norm > 0.8f) {
                    // Begin spiral toward center
                    float spiral_progress = (t_norm - 0.8f) / 0.2f;
                    current_radius = mixf(current_radius, 1.0f, spiral_progress);
                }
                
                // Set the orb position
                polar_t newPos = polar_t::Degrees(target_angle, current_radius);
                newPos.normalize();
                orb->SetOrigin(newPos);
                
                // Update the orb color based on transition progress
                HSV currentHSV = interpolateHSV(hsv_from[k], hsv_to[k], color_blend);
                orb->SetColor(hsv2rgb(currentHSV));
                
                // Update speeds for dynamics
                if (phase == FUSION || phase == EXPANSION) {
                    // Faster speed during fusion/expansion
                    orb->rot_speed = orb_speeds[k] * 1.2f;
                } else {
                    // Normal speed during other phases
                    orb->rot_speed = orb_speeds[k];
                }
            }
        }
    
        // Override the required Draw method from Animatable
        void Draw(LEDMatrix* matrix) override {
            // This is just a stub that redirects to our custom Draw method with LEDs
            // This is needed because we inherit from Animatable which has a pure virtual Draw method
        }
        
        // Our custom Draw method that takes additional parameters
        void DrawTransition(LEDMatrix* matrix, std::array<led_color_t, LED_COUNT>& leds, const std::array<polar_t, LED_COUNT>& led_lut) {
            // If in FLASH phase, create a bright flash effect
            if (phase == FLASH) {
                // Flash phase: pulse white with subtle color undertones
                float flashIntensity = 1.0f;
                if (t_phase < T_flash * 0.5f) {
                    flashIntensity = t_phase / (T_flash * 0.5f);
                } else {
                    flashIntensity = 1.0f - ((t_phase - T_flash * 0.5f) / (T_flash * 0.5f));
                }
                
                // Create a bright white/color blend with subtle color hints
                for (int i = 0; i < LED_COUNT; ++i) {
                    // Get a blend of all three orb colors for a richer flash effect
                    led_color_t base_color1 = dynamic_cast<Orb*>(orbs[0].get())->color;
                    led_color_t base_color2 = dynamic_cast<Orb*>(orbs[1].get())->color;
                    led_color_t base_color3 = dynamic_cast<Orb*>(orbs[2].get())->color;
                    
                    // Average the colors and add white for flash
                    uint8_t r = static_cast<uint8_t>((base_color1.r + base_color2.r + base_color3.r) / 3);
                    uint8_t g = static_cast<uint8_t>((base_color1.g + base_color2.g + base_color3.g) / 3);
                    uint8_t b = static_cast<uint8_t>((base_color1.b + base_color2.b + base_color3.b) / 3);
                    
                    // Blend with white for flash effect
                    r = static_cast<uint8_t>(r * 0.3f + 255 * 0.7f * flashIntensity);
                    g = static_cast<uint8_t>(g * 0.3f + 255 * 0.7f * flashIntensity);
                    b = static_cast<uint8_t>(b * 0.3f + 255 * 0.7f * flashIntensity);
                    
                    leds[i] = {r, g, b};
                }
                
                return;
            }
            
            // For all other phases, use the Gaussian blending from run() function
            // Reset the LED buffer for this frame
            for (int i = 0; i < LED_COUNT; ++i) {
                leds[i] = {0, 0, 0};
            }
            
            // Apply Gaussian blending for each orb (similar to the active state)
            for (size_t o = 0; o < orbs.size(); ++o) {
                auto orbPtr = dynamic_cast<Orb*>(orbs[o].get());
                polar_t C = orbPtr->GetOrigin();
                
                // For each LED in the matrix
                for (int i = 0; i < LED_COUNT; ++i) {
                    polar_t P = led_lut[i];
                    
                    // Calculate Gaussian influence based on distance
                    float dθ = angularDifference(P.theta, C.theta);
                    float r̄ = (P.r + C.r) * 0.5f;
                    float Δr = P.r - C.r;
                    float d2 = (dθ * r̄)*(dθ * r̄) + (Δr * Δr);
                    float F = std::exp(-d2 / (2 * sigma[o] * sigma[o]));
                    
                    // Get current color of the orb 
                    led_color_t base = orbPtr->color;
                    
                    // Apply intensity and falloff
                    led_color_t contrib = base * (intensity[o] * F);
                    
                    // Additive blend (clamped at 255)
                    leds[i] = leds[i] + contrib;
                }
            }
        }
        
    private:
        std::array<HSV, 3> hsv_from;
        std::array<HSV, 3> hsv_to;
        std::vector<std::unique_ptr<Orb>> orbs;
        std::vector<float> orb_speeds;
        std::vector<float> sigma;        // Gaussian blur radius for each orb
        std::vector<float> intensity;    // Intensity multiplier for each orb
        std::vector<polar_t> initial_positions; // Store initial positions
        
        Phase phase;
        float t_phase;
        float dt;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
        std::chrono::time_point<std::chrono::high_resolution_clock> phase_start_time;
    };