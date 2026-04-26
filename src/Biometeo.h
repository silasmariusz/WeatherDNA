#ifndef BIOMETEO_H
#define BIOMETEO_H

#include <Arduino.h>
#include <math.h>
#include <time.h>

#define BIOMETEO_HIST_SIZE 18

class BiometeoCore {
private:
    float p_hist_hPa[BIOMETEO_HIST_SIZE] = {0};
    uint64_t p_hist_ts[BIOMETEO_HIST_SIZE] = {0};
    int p_hist_idx = 0;

public:
    BiometeoCore() {}

    float calcDewPoint(float t, float h) { return t - ((100.0f - h) / 5.0f); }
    
    float calcSLP(float p_abs, float t, float alt) { 
        return p_abs * pow((1.0f - (0.0065f * alt) / (t + 0.0065f * alt + 273.15f)), -5.257f); 
    }
    
    float calcAbsoluteHumidity(float t, float h) { 
        return (6.112f * exp((17.67f * t) / (t + 243.5f)) * h * 2.1674f) / (273.15f + t); 
    }
    
    float calcHeatIndex(float t, float h) {
        if (t <= 27.0f || h <= 40.0f) return t;
        return t + 0.33f * (h / 100.0f * 6.105f * exp(17.27f * t / (237.7f + t))) - 4.0f;
    }
    
    float calcSmogIndex(float pm25, float nox, float hum) {
        float pm_factor = pm25 / 10.0f;
        float hum_mult = 1.0f + (max(0.0f, hum - 70.0f) / 100.0f);
        return min(10.0f, (pm_factor * hum_mult) + (nox / 50.0f));
    }

    void updatePressureBuffer(float current_p) {
        uint64_t now_sec = time(nullptr);
        if (now_sec < 1600000000) return; // Zignoruj jeśli brak synch z NTP
        if (now_sec - p_hist_ts[p_hist_idx] >= 600) {
            p_hist_idx = (p_hist_idx + 1) % BIOMETEO_HIST_SIZE;
            p_hist_hPa[p_hist_idx] = current_p;
            p_hist_ts[p_hist_idx] = now_sec;
        }
    }

    float getPressureDelta3h(float current_p) {
        uint64_t now_sec = time(nullptr); 
        float oldest_p = current_p;
        uint64_t target_ts = now_sec - (3 * 3600); // Ciśnienie sprzed 3 godzin
        for (int i = 0; i < BIOMETEO_HIST_SIZE; i++) {
            if (p_hist_ts[i] > 0 && abs((long long)p_hist_ts[i] - (long long)target_ts) < 1800) {
                oldest_p = p_hist_hPa[i]; break;
            }
        }
        return current_p - oldest_p;
    }

    String calcZambretti(float slp_hPa, float delta3h_hPa, int month) {
        bool rising = (delta3h_hPa > 0.5f);
        bool falling = (delta3h_hPa < -0.5f);
        if (falling) {
            if (slp_hPa > 1020) return "Settled, worsening";
            if (slp_hPa > 1010) return "Changeable, rain later";
            if (slp_hPa > 1000) return "Rain, wind";
            return "Stormy, heavy rain";
        } else if (rising) {
            if (slp_hPa > 1020) return "Settled, fine";
            if (slp_hPa > 1010) return "Becoming fine";
            if (slp_hPa > 1000) return "Showery, improving";
            return "Unsettled, improving";
        } else {
            if (slp_hPa > 1015) return "Fine, stable";
            if (slp_hPa > 1005) return "Fairly fine";
            return "Showery, unsettled";
        }
    }
};

#endif