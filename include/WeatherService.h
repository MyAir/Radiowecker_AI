#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"

class WeatherService {
public:
    // Weather data structures
    struct CurrentWeather {
        long dt = 0;
        long sunrise = 0;
        long sunset = 0;
        float temp = 0.0f;
        float feels_like = 0.0f;
        int pressure = 0;
        int humidity = 0;
        float dew_point = 0.0f;
        int clouds = 0;
        float uvi = 0.0f;
        int visibility = 0;
        float wind_speed = 0.0f;
        float wind_gust = 0.0f;
        int wind_deg = 0;
        float rain_1h = 0.0f;
        float snow_1h = 0.0f;
        int weather_id = 0;
        String weather_main;
        String weather_description;
        String weather_icon;
    };
    
    struct DailyForecast {
        long dt = 0;
        long sunrise = 0;
        long sunset = 0;
        long moonrise = 0;
        long moonset = 0;
        float moon_phase = 0.0f;
        
        struct {
            float day = 0.0f;
            float min = 0.0f;
            float max = 0.0f;
            float night = 0.0f;
            float eve = 0.0f;
            float morn = 0.0f;
        } temp;
        
        struct {
            float day = 0.0f;
            float night = 0.0f;
            float eve = 0.0f;
            float morn = 0.0f;
        } feels_like;
        
        int pressure = 0;
        int humidity = 0;
        float dew_point = 0.0f;
        float wind_speed = 0.0f;
        float wind_gust = 0.0f;
        int wind_deg = 0;
        int clouds = 0;
        float uvi = 0.0f;
        float pop = 0.0f; // Probability of precipitation
        float rain = 0.0f;
        float snow = 0.0f;
        int weather_id = 0;
        String weather_main;
        String weather_description;
        String weather_icon;
    };
    
private:
    static WeatherService* instance;
    
    // Private constructor
    WeatherService() = default;
    
    // Prevent copying and assignment
    WeatherService(const WeatherService&) = delete;
    WeatherService& operator=(const WeatherService&) = delete;
    
    // API call details
    String appid;
    float lat;
    float lon;
    String units;
    String lang;
    uint32_t lastUpdateTime = 0;
    uint32_t updateInterval = 300000; // Default 5 minutes (300,000 ms)
    
    // Weather data storage
    CurrentWeather currentWeather;
    DailyForecast dailyForecasts[8]; // 8 days forecast (today + 7 days)
    
    // Function to make API call
    bool fetchWeatherData();
    
    // Parse different parts of the API response
    void parseCurrentWeather(JsonObject& current);
    void parseDailyForecast(JsonArray& daily);

public:
    // Static method to get the singleton instance
    static WeatherService& getInstance() {
        if (!instance) {
            instance = new WeatherService();
        }
        return *instance;
    }
    
    // Initialize with config
    bool init();
    
    // Update weather data (will only fetch if updateInterval has passed)
    bool update();
    
    // Force update regardless of time interval
    bool forceUpdate();
    
    // Getters for weather data
    const CurrentWeather& getCurrentWeather() const { return currentWeather; }
    const DailyForecast& getDailyForecast(int day) const { 
        if (day >= 0 && day < 8) {
            return dailyForecasts[day];
        }
        return dailyForecasts[0]; // Return today's forecast as fallback
    }
    
    // Set update interval (in milliseconds)
    void setUpdateInterval(uint32_t interval) { updateInterval = interval; }
};

// Declaration of static instance pointer
// Will be defined in the cpp file

#endif // WEATHERSERVICE_H
