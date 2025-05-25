#include "WeatherService.h"

bool WeatherService::init() {
    ConfigManager& config = ConfigManager::getInstance();
    WeatherConfig weatherConfig = config.getWeatherConfig();
    
    // Store configuration
    apiKey = weatherConfig.api_key;
    location = weatherConfig.city_id;
    units = weatherConfig.units.isEmpty() ? "metric" : weatherConfig.units;
    
    // Set update interval if configured
    if (weatherConfig.update_interval > 0) {
        updateInterval = weatherConfig.update_interval * 60 * 1000; // Convert minutes to milliseconds
    }
    
    // Check if we have the necessary configuration
    if (apiKey.isEmpty() || location.isEmpty()) {
        Serial.println("[ERROR] Weather API key or location not configured");
        return false;
    }
    
    Serial.println("[INFO] WeatherService initialized successfully");
    return true;
}

bool WeatherService::update() {
    uint32_t currentTime = millis();
    
    // Only update if enough time has passed since last update
    if (currentTime - lastUpdateTime >= updateInterval) {
        return fetchWeatherData();
    }
    
    return true; // No update needed
}

bool WeatherService::forceUpdate() {
    return fetchWeatherData();
}

bool WeatherService::fetchWeatherData() {
    if (apiKey.isEmpty() || location.isEmpty()) {
        Serial.println("[ERROR] Weather API key or location not configured");
        return false;
    }
    
    HTTPClient http;
    
    // Construct the One Call API URL for OpenWeatherMap
    // Format: https://api.openweathermap.org/data/3.0/onecall?lat={lat}&lon={lon}&appid={API key}&units={units}
    
    String url;
    
    // Check if location is a city ID (numeric) or coordinates (lat,lon)
    if (location.indexOf(",") > 0) {
        // Format is "lat,lon"
        int commaPos = location.indexOf(",");
        String lat = location.substring(0, commaPos);
        String lon = location.substring(commaPos + 1);
        
        // Construct URL with lat/lon
        url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + lat + 
              "&lon=" + lon + 
              "&appid=" + apiKey + 
              "&units=" + units + 
              "&exclude=minutely,alerts";
    } else {
        // Assume it's a city ID - we need to first get geocoding for the city
        String geocodeUrl = "https://api.openweathermap.org/geo/1.0/direct?q=" + location + 
                           "&limit=1&appid=" + apiKey;
        
        http.begin(geocodeUrl);
        int httpCode = http.GET();
        
        if (httpCode != HTTP_CODE_OK) {
            Serial.printf("[ERROR] Geocoding API failed, code: %d\n", httpCode);
            http.end();
            return false;
        }
        
        String payload = http.getString();
        http.end();
        
        // Parse geocoding response to get lat/lon
        DynamicJsonDocument geocodeDoc(1024);
        DeserializationError error = deserializeJson(geocodeDoc, payload);
        
        if (error) {
            Serial.printf("[ERROR] Geocoding JSON parsing failed: %s\n", error.c_str());
            return false;
        }
        
        if (geocodeDoc.size() == 0) {
            Serial.println("[ERROR] City not found in geocoding API");
            return false;
        }
        
        float lat = geocodeDoc[0]["lat"];
        float lon = geocodeDoc[0]["lon"];
        
        // Construct URL with lat/lon
        url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + String(lat, 6) + 
              "&lon=" + String(lon, 6) + 
              "&appid=" + apiKey + 
              "&units=" + units + 
              "&exclude=minutely,alerts";
    }
    
    // Make the API request
    Serial.println("[INFO] Fetching weather data from: " + url);
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[ERROR] Weather API request failed, code: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse JSON response
    DynamicJsonDocument doc(16384); // Large document for the complete response
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf("[ERROR] Weather JSON parsing failed: %s\n", error.c_str());
        return false;
    }
    
    // Parse current weather
    JsonObject current = doc["current"];
    if (current) {
        parseCurrentWeather(current);
    }
    
    // Parse forecast data for morning and afternoon
    JsonArray hourly = doc["hourly"];
    if (hourly) {
        parseForecast(hourly);
    }
    
    // Update the UI
    UIManager& ui = UIManager::getInstance();
    ui.updateCurrentWeather(
        weatherData.currentIcon.c_str(),
        weatherData.currentTemp,
        weatherData.feelsLike
    );
    
    ui.updateMorningForecast(
        weatherData.morningIcon.c_str(),
        weatherData.morningTemp,
        weatherData.morningRainProb
    );
    
    ui.updateAfternoonForecast(
        weatherData.afternoonIcon.c_str(),
        weatherData.afternoonTemp,
        weatherData.afternoonRainProb
    );
    
    // Update last update time
    lastUpdateTime = millis();
    
    Serial.println("[INFO] Weather data updated successfully");
    return true;
}

void WeatherService::parseCurrentWeather(JsonObject& current) {
    // Extract current temperature
    weatherData.currentTemp = current["temp"];
    
    // Extract feels like temperature
    weatherData.feelsLike = current["feels_like"];
    
    // Extract weather icon
    if (current["weather"][0]) {
        weatherData.currentIcon = current["weather"][0]["icon"].as<String>();
    }
    
    Serial.printf("[INFO] Current weather: %.1f°C (feels like %.1f°C), icon: %s\n", 
                 weatherData.currentTemp, weatherData.feelsLike, weatherData.currentIcon.c_str());
}

void WeatherService::parseForecast(JsonArray& hourly) {
    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    int currentHour = timeinfo.tm_hour;
    
    // Find forecast entries for 9 AM and 3 PM today
    int morningHour = 9;
    int afternoonHour = 15;
    
    // If current hour is past noon, use tomorrow's morning forecast
    bool useTomorrowMorning = (currentHour >= 12);
    
    // If current hour is past afternoon, use tomorrow's afternoon forecast
    bool useTomorrowAfternoon = (currentHour >= 16);
    
    int targetMorningIndex = -1;
    int targetAfternoonIndex = -1;
    
    for (int i = 0; i < hourly.size() && i < 48; i++) { // Look ahead up to 48 hours
        // Get forecast time
        time_t forecastTime = hourly[i]["dt"];
        struct tm forecastTm;
        localtime_r(&forecastTime, &forecastTm);
        
        // Check if this is the morning forecast we want
        if (forecastTm.tm_hour == morningHour) {
            if (!useTomorrowMorning || (useTomorrowMorning && forecastTm.tm_mday > timeinfo.tm_mday)) {
                targetMorningIndex = i;
                if (targetAfternoonIndex >= 0) break; // Found both, exit loop
            }
        }
        
        // Check if this is the afternoon forecast we want
        if (forecastTm.tm_hour == afternoonHour) {
            if (!useTomorrowAfternoon || (useTomorrowAfternoon && forecastTm.tm_mday > timeinfo.tm_mday)) {
                targetAfternoonIndex = i;
                if (targetMorningIndex >= 0) break; // Found both, exit loop
            }
        }
    }
    
    // Parse morning forecast if found
    if (targetMorningIndex >= 0) {
        JsonObject morningForecast = hourly[targetMorningIndex];
        weatherData.morningTemp = morningForecast["temp"];
        weatherData.morningRainProb = morningForecast["pop"].as<float>() * 100; // Convert 0-1 to percentage
        
        if (morningForecast["weather"][0]) {
            weatherData.morningIcon = morningForecast["weather"][0]["icon"].as<String>();
        }
        
        Serial.printf("[INFO] Morning forecast: %.1f°C, rain: %d%%, icon: %s\n", 
                     weatherData.morningTemp, weatherData.morningRainProb, weatherData.morningIcon.c_str());
    }
    
    // Parse afternoon forecast if found
    if (targetAfternoonIndex >= 0) {
        JsonObject afternoonForecast = hourly[targetAfternoonIndex];
        weatherData.afternoonTemp = afternoonForecast["temp"];
        weatherData.afternoonRainProb = afternoonForecast["pop"].as<float>() * 100; // Convert 0-1 to percentage
        
        if (afternoonForecast["weather"][0]) {
            weatherData.afternoonIcon = afternoonForecast["weather"][0]["icon"].as<String>();
        }
        
        Serial.printf("[INFO] Afternoon forecast: %.1f°C, rain: %d%%, icon: %s\n", 
                     weatherData.afternoonTemp, weatherData.afternoonRainProb, weatherData.afternoonIcon.c_str());
    }
}
