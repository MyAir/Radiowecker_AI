#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "UIManager.h"

class WeatherService {
private:
    static WeatherService* instance;
    
    // Private constructor
    WeatherService() = default;
    
    // Prevent copying and assignment
    WeatherService(const WeatherService&) = delete;
    WeatherService& operator=(const WeatherService&) = delete;
    
    // API call details
    String apiKey;
    String location;
    String units;
    uint32_t lastUpdateTime = 0;
    uint32_t updateInterval = 300000; // Default 5 minutes (300,000 ms)
    
    // Weather data
    struct WeatherData {
        // Current weather
        float currentTemp = 0.0f;
        float feelsLike = 0.0f;
        String currentIcon = "";
        
        // Morning forecast (9 AM)
        float morningTemp = 0.0f;
        int morningRainProb = 0;
        String morningIcon = "";
        
        // Afternoon forecast (15 PM)
        float afternoonTemp = 0.0f;
        int afternoonRainProb = 0;
        String afternoonIcon = "";
    };
    
    WeatherData weatherData;
    
    // Function to make API call
    bool fetchWeatherData();
    
    // Parse different parts of the API response
    void parseCurrentWeather(JsonObject& current);
    void parseForecast(JsonArray& hourly);
    
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
    float getCurrentTemp() const { return weatherData.currentTemp; }
    float getFeelsLike() const { return weatherData.feelsLike; }
    const String& getCurrentIcon() const { return weatherData.currentIcon; }
    
    float getMorningTemp() const { return weatherData.morningTemp; }
    int getMorningRainProb() const { return weatherData.morningRainProb; }
    const String& getMorningIcon() const { return weatherData.morningIcon; }
    
    float getAfternoonTemp() const { return weatherData.afternoonTemp; }
    int getAfternoonRainProb() const { return weatherData.afternoonRainProb; }
    const String& getAfternoonIcon() const { return weatherData.afternoonIcon; }
    
    // Set update interval (in milliseconds)
    void setUpdateInterval(uint32_t interval) { updateInterval = interval; }
};

// Declaration of static instance
inline WeatherService* WeatherService::instance = nullptr;

#endif // WEATHERSERVICE_H
