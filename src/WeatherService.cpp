#include "WeatherService.h"

// Define the static instance pointer
WeatherService* WeatherService::instance = nullptr;

bool WeatherService::init() {
    ConfigManager& configManager = ConfigManager::getInstance();
    WeatherConfig weatherConfig = configManager.getWeatherConfig();
    
    // Get the API key
    appid = weatherConfig.appid;
    
    // Check if API key is set
    if (appid.isEmpty() || appid == "") {
        Serial.println("[ERROR] Weather API key (appid) is empty in config");
        return false;
    }
    
    // Get lat/lon coordinates
    lat = weatherConfig.lat;
    lon = weatherConfig.lon;
    
    // Check if coordinates are set
    if (lat == 0 && lon == 0) {
        Serial.println("[ERROR] Weather location (lat/lon) not configured");
        return false;
    }
    
    // Get units (default to metric if not set)
    units = weatherConfig.units;
    if (units.isEmpty()) {
        units = "metric";
    }
    
    // Get language (default to German if not set)
    lang = weatherConfig.lang;
    if (lang.isEmpty()) {
        lang = "de"; // Default to German as per user preference
    }
    
    Serial.println("[INFO] WeatherService initialized successfully");
    Serial.printf("[INFO] Weather config: API key=%s, lat=%.6f, lon=%.6f, units=%s, lang=%s\n", 
                 appid.c_str(), lat, lon, units.c_str(), lang.c_str());
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
    if (appid.isEmpty()) {
        Serial.println("[ERROR] Weather API key not configured");
        return false;
    }
    
    HTTPClient http;
    
    // Construct the One Call API URL for OpenWeatherMap
    // Format: https://api.openweathermap.org/data/3.0/onecall?lat={lat}&lon={lon}&appid={API key}&units={units}&lang={lang}
    String url = "https://api.openweathermap.org/data/3.0/onecall"
                 "?lat=" + String(lat, 6) + 
                 "&lon=" + String(lon, 6) + 
                 "&appid=" + appid + 
                 "&units=" + units + 
                 "&lang=" + lang + 
                 "&exclude=minutely,hourly,alerts";
    
    // Make the API request
    Serial.println("[INFO] Fetching weather data from OpenWeatherMap API");
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[ERROR] Weather API request failed, code: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    Serial.println("[INFO] Weather data received, parsing...");
    
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
    } else {
        Serial.println("[WARNING] No current weather data found in API response");
    }
    
    // Parse daily forecasts
    JsonArray daily = doc["daily"];
    if (daily) {
        parseDailyForecast(daily);
    } else {
        Serial.println("[WARNING] No daily forecast data found in API response");
    }
    
    // Update last update time
    lastUpdateTime = millis();
    
    Serial.println("[INFO] Weather data updated successfully");
    return true;
}

void WeatherService::parseCurrentWeather(JsonObject& current) {
    // Reset to defaults
    currentWeather = CurrentWeather();
    
    // Parse main fields
    currentWeather.dt = current["dt"].as<long>();
    currentWeather.sunrise = current["sunrise"].as<long>();
    currentWeather.sunset = current["sunset"].as<long>();
    currentWeather.temp = current["temp"].as<float>();
    currentWeather.feels_like = current["feels_like"].as<float>();
    currentWeather.pressure = current["pressure"].as<int>();
    currentWeather.humidity = current["humidity"].as<int>();
    currentWeather.dew_point = current["dew_point"].as<float>();
    currentWeather.clouds = current["clouds"].as<int>();
    currentWeather.uvi = current["uvi"].as<float>();
    currentWeather.visibility = current["visibility"].as<int>();
    currentWeather.wind_speed = current["wind_speed"].as<float>();
    currentWeather.wind_gust = current["wind_gust"].as<float>();
    currentWeather.wind_deg = current["wind_deg"].as<int>();
    
    // Parse optional precipitation fields
    if (current.containsKey("rain") && current["rain"].containsKey("1h")) {
        currentWeather.rain_1h = current["rain"]["1h"].as<float>();
    }
    
    if (current.containsKey("snow") && current["snow"].containsKey("1h")) {
        currentWeather.snow_1h = current["snow"]["1h"].as<float>();
    }
    
    // Parse weather condition
    if (current.containsKey("weather") && current["weather"].size() > 0) {
        JsonObject weather = current["weather"][0];
        currentWeather.weather_id = weather["id"].as<int>();
        currentWeather.weather_main = weather["main"].as<String>();
        currentWeather.weather_description = weather["description"].as<String>();
        currentWeather.weather_icon = weather["icon"].as<String>();
    }
    
    Serial.printf("[INFO] Current weather: %.1f°C (feels like %.1f°C), %s\n", 
                  currentWeather.temp, currentWeather.feels_like, 
                  currentWeather.weather_description.c_str());
}

void WeatherService::parseDailyForecast(JsonArray& daily) {
    // Get the minimum of the array size and our storage capacity
    int count = min(daily.size(), (size_t)8);
    
    for (int i = 0; i < count; i++) {
        // Reset to defaults
        dailyForecasts[i] = DailyForecast();
        
        JsonObject forecast = daily[i];
        
        // Parse main fields
        dailyForecasts[i].dt = forecast["dt"].as<long>();
        dailyForecasts[i].sunrise = forecast["sunrise"].as<long>();
        dailyForecasts[i].sunset = forecast["sunset"].as<long>();
        dailyForecasts[i].moonrise = forecast["moonrise"].as<long>();
        dailyForecasts[i].moonset = forecast["moonset"].as<long>();
        dailyForecasts[i].moon_phase = forecast["moon_phase"].as<float>();
        
        // Parse temperature
        JsonObject temp = forecast["temp"];
        if (temp) {
            dailyForecasts[i].temp.day = temp["day"].as<float>();
            dailyForecasts[i].temp.min = temp["min"].as<float>();
            dailyForecasts[i].temp.max = temp["max"].as<float>();
            dailyForecasts[i].temp.night = temp["night"].as<float>();
            dailyForecasts[i].temp.eve = temp["eve"].as<float>();
            dailyForecasts[i].temp.morn = temp["morn"].as<float>();
        }
        
        // Parse feels like
        JsonObject feels_like = forecast["feels_like"];
        if (feels_like) {
            dailyForecasts[i].feels_like.day = feels_like["day"].as<float>();
            dailyForecasts[i].feels_like.night = feels_like["night"].as<float>();
            dailyForecasts[i].feels_like.eve = feels_like["eve"].as<float>();
            dailyForecasts[i].feels_like.morn = feels_like["morn"].as<float>();
        }
        
        // Parse other fields
        dailyForecasts[i].pressure = forecast["pressure"].as<int>();
        dailyForecasts[i].humidity = forecast["humidity"].as<int>();
        dailyForecasts[i].dew_point = forecast["dew_point"].as<float>();
        dailyForecasts[i].wind_speed = forecast["wind_speed"].as<float>();
        dailyForecasts[i].wind_gust = forecast["wind_gust"].as<float>();
        dailyForecasts[i].wind_deg = forecast["wind_deg"].as<int>();
        dailyForecasts[i].clouds = forecast["clouds"].as<int>();
        dailyForecasts[i].uvi = forecast["uvi"].as<float>();
        dailyForecasts[i].pop = forecast["pop"].as<float>();
        
        // Parse optional precipitation
        if (forecast.containsKey("rain")) {
            dailyForecasts[i].rain = forecast["rain"].as<float>();
        }
        
        if (forecast.containsKey("snow")) {
            dailyForecasts[i].snow = forecast["snow"].as<float>();
        }
        
        // Parse weather condition
        if (forecast.containsKey("weather") && forecast["weather"].size() > 0) {
            JsonObject weather = forecast["weather"][0];
            dailyForecasts[i].weather_id = weather["id"].as<int>();
            dailyForecasts[i].weather_main = weather["main"].as<String>();
            dailyForecasts[i].weather_description = weather["description"].as<String>();
            dailyForecasts[i].weather_icon = weather["icon"].as<String>();
        }
        
        Serial.printf("[INFO] Daily forecast %d: %.1f°C (min: %.1f°C, max: %.1f°C), rain prob: %.0f%%, %s\n", 
                      i, dailyForecasts[i].temp.day, dailyForecasts[i].temp.min, 
                      dailyForecasts[i].temp.max, dailyForecasts[i].pop * 100, 
                      dailyForecasts[i].weather_description.c_str());
    }
}
