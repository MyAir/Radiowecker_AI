#include "WeatherService.h"
#include <ArduinoJson.h>

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
    
    // 1. Prepare the URL for OpenWeatherMap API request
    String url = "https://api.openweathermap.org/data/3.0/onecall"
                 "?lat=" + String(lat, 6) + 
                 "&lon=" + String(lon, 6) + 
                 "&appid=" + appid + 
                 "&units=" + units + 
                 "&lang=" + lang;

    // We'll parse each section separately instead of the entire payload at once
    bool currentSuccess = false;
    bool dailySuccess = false;
    bool hourlySuccess = false;
    
    // 2. First fetch just the current weather (exclude everything else)
    {
        HTTPClient http;
        String currentUrl = url + "&exclude=minutely,hourly,daily,alerts";
        
        Serial.println("[INFO] Fetching current weather data");
        http.begin(currentUrl);
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            // Parse only the current weather data
            DynamicJsonDocument doc(2048); // Smaller buffer just for current weather
            DeserializationError error = deserializeJson(doc, http.getString());
            
            if (!error) {
                if (doc.containsKey("current")) {
                    parseCurrentWeather(doc["current"]);
                    currentSuccess = true;
                } else {
                    Serial.println("[WARNING] No current weather data found in API response");
                }
            } else {
                Serial.printf("[ERROR] Current weather JSON parsing failed: %s\n", error.c_str());
            }
        } else {
            Serial.printf("[ERROR] Current weather API request failed, code: %d\n", httpCode);
        }
        
        http.end();
    }
    
    // 3. Now fetch daily forecast data (exclude everything else)
    {
        HTTPClient http;
        String dailyUrl = url + "&exclude=current,minutely,hourly,alerts";
        
        Serial.println("[INFO] Fetching daily forecast data");
        http.begin(dailyUrl);
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            // Parse only the daily forecast data
            DynamicJsonDocument doc(4096); // Buffer for daily forecasts
            DeserializationError error = deserializeJson(doc, http.getString());
            
            if (!error) {
                if (doc.containsKey("daily") && doc["daily"].is<JsonArray>()) {
                    int processedDays = 0;
                    JsonArray dailyArray = doc["daily"];
                    
                    for (int i = 0; i < dailyArray.size() && i < 8; i++) {
                        JsonObject dailyForecast = dailyArray[i];
                        DailyForecast& forecast = dailyForecasts[i];
                        
                        // Parse only essential fields we need
                        forecast.dt = dailyForecast["dt"].as<long>();
                        forecast.sunrise = dailyForecast["sunrise"].as<long>();
                        forecast.sunset = dailyForecast["sunset"].as<long>();
                        
                        // Parse temperature data
                        if (dailyForecast.containsKey("temp")) {
                            JsonObject temp = dailyForecast["temp"];
                            forecast.temp.day = temp["day"].as<float>();
                            forecast.temp.min = temp["min"].as<float>();
                            forecast.temp.max = temp["max"].as<float>();
                            forecast.temp.night = temp["night"].as<float>();
                            forecast.temp.eve = temp["eve"].as<float>();
                            forecast.temp.morn = temp["morn"].as<float>();
                        }
                        
                        // Parse weather information
                        forecast.pop = dailyForecast["pop"].as<float>();
                        if (dailyForecast.containsKey("weather") && dailyForecast["weather"].is<JsonArray>()) {
                            JsonArray weatherArray = dailyForecast["weather"];
                            if (weatherArray.size() > 0) {
                                JsonObject weather = weatherArray[0];
                                forecast.weather_icon = weather["icon"].as<String>();
                                forecast.weather_description = weather["description"].as<String>();
                            }
                        }
                        processedDays++;
                    }
                    
                    Serial.printf("[INFO] Successfully parsed %d daily forecasts\n", processedDays);
                    dailySuccess = (processedDays > 0);
                } else {
                    Serial.println("[WARNING] No daily forecast data found in API response");
                }
            } else {
                Serial.printf("[ERROR] Daily forecast JSON parsing failed: %s\n", error.c_str());
            }
        } else {
            Serial.printf("[ERROR] Daily forecast API request failed, code: %d\n", httpCode);
        }
        
        http.end();
    }
    
    // 4. Finally, fetch the hourly forecast (this is our primary data)
    {
        HTTPClient http;
        String hourlyUrl = url + "&exclude=current,minutely,daily,alerts";
        
        Serial.println("[INFO] Fetching hourly forecast data");
        http.begin(hourlyUrl);
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            // Clear hourly forecasts array before repopulating
            memset(hourlyForecasts, 0, sizeof(hourlyForecasts));
            hourlyForecastCount = 0;
            hourlySuccess = false; // Start with false, set to true if we get any data
            
            // Process hourly data with an ultra-minimal approach that avoids memory issues
            Serial.println("[INFO] Processing hourly data with ultra-minimal memory approach");
            
            // Instead of parsing the whole JSON at once, we'll process the hourly entries one at a time
            String response = http.getString();
            Serial.printf("[DEBUG] Hourly response size: %d bytes\n", response.length());
            
            // First find the hourly array in the JSON
            int hourlyPos = response.indexOf("\"hourly\":");
            if (hourlyPos > 0) {
                Serial.printf("[DEBUG] Found hourly field at position %d\n", hourlyPos);
                hourlyPos += 9; // Skip past "hourly":

                // Find the start of the array
                int arrayStart = response.indexOf('[', hourlyPos);
                if (arrayStart > 0) {
                    Serial.printf("[DEBUG] Found hourly array start at position %d\n", arrayStart);
            
                    // Process up to 24 hours (one day), which is all we need
                    const int MAX_TO_PROCESS = 24;
                    
                    // Process each hourly entry manually
                    int pos = arrayStart + 1; // Skip the opening bracket
                    int entryStart = 0;
                    int entryEnd = 0;
                    int bracketCount = 1; // We're already inside one level of brackets
                
                    for (int i = 0; i < MAX_TO_PROCESS && hourlyForecastCount < MAX_HOURLY_FORECASTS && pos < response.length(); i++) {
                        // Find start of this entry (should be a '{')
                        while (pos < response.length() && response[pos] != '{') pos++;
                        if (pos >= response.length()) break;
                        
                        entryStart = pos;
                        bracketCount = 1; // We're about to enter a nested object
                        
                        // Find the matching closing brace for this entry
                        pos++; // Skip past the opening brace
                        while (pos < response.length() && bracketCount > 0) {
                            if (response[pos] == '{') bracketCount++;
                            else if (response[pos] == '}') bracketCount--;
                            pos++;
                        }
                        
                        if (bracketCount == 0) { // Found the complete entry
                            entryEnd = pos;
                            
                            // Extract just this one entry and parse it
                            String entryJson = "{" + response.substring(entryStart + 1, entryEnd - 1) + "}";
                            
                            // Create a minimal filter just for fields we need
                            StaticJsonDocument<128> entryFilter;
                            entryFilter["dt"] = true;
                            entryFilter["temp"] = true;
                            entryFilter["pop"] = true;
                            entryFilter["weather"][0]["icon"] = true;
                            
                            // Parse this one entry with a tiny document
                            DynamicJsonDocument entryDoc(512);
                            DeserializationError error = deserializeJson(
                                entryDoc, 
                                entryJson,
                                DeserializationOption::Filter(entryFilter));
                                
                            if (!error) {
                                // Extract forecast data for this hour
                                HourlyForecast forecast;
                                forecast.dt = entryDoc["dt"].as<long>();
                                forecast.temp = entryDoc["temp"].as<float>();
                                forecast.pop = entryDoc["pop"].as<float>();
                                
                                // Get weather icon if available
                                if (!entryDoc["weather"][0]["icon"].isNull()) {
                                    forecast.weather_icon = entryDoc["weather"][0]["icon"].as<String>();
                                }
                                
                                // Add to our array
                                hourlyForecasts[hourlyForecastCount++] = forecast;
                                hourlySuccess = true; // We got at least one entry
                                
                                // Debug output for this entry
                                Serial.printf("[DEBUG] Hour %d: %ld, %.1f°C, %.0f%%, icon=%s\n",
                                            i, forecast.dt, forecast.temp, forecast.pop * 100, 
                                            forecast.weather_icon.c_str());
                                
                                // Convert timestamp to readable format
                                struct tm timeinfo;
                                time_t timestamp = forecast.dt;
                                localtime_r(&timestamp, &timeinfo);
                                char timeStr[20];
                                strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &timeinfo);
                                Serial.printf("[DEBUG] Hour %d time: %s\n", i, timeStr);
                            } else {
                                Serial.printf("[DEBUG] Error parsing hourly entry %d: %s\n", i, error.c_str());
                            }
                        } else {
                            // Unbalanced braces, something went wrong
                            Serial.println("[ERROR] Unbalanced braces in JSON parsing");
                            break;
                        }
                    }
                    
                    Serial.printf("[INFO] Successfully processed %d hourly entries\n", hourlyForecastCount);
                    
                    // If we got data, process the forecasts
                    if (hourlyForecastCount > 0) {
                        calculateDailyForecasts();
                    } else {
                        hourlySuccess = false;
                        Serial.println("[ERROR] No hourly forecast entries could be parsed");
                    }
                } else {
                    Serial.println("[ERROR] Could not find hourly array start in JSON");
                    hourlySuccess = false;
                }
            } else {
                Serial.println("[ERROR] Could not find hourly data in JSON response");
                hourlySuccess = false;
            }
        } else {
            Serial.printf("[ERROR] Hourly forecast API request failed with error code: %d\n", httpCode);
            hourlySuccess = false;
        }
        
        http.end();
    }
    
    // Update the last update time if any part succeeded
    if (currentSuccess || dailySuccess || hourlySuccess) {
        lastUpdateTime = millis();
        Serial.println("[INFO] Weather data updated successfully");
        calculateDailyForecasts(); // Only calculate forecasts if we have data
        return true;
    } else {
        Serial.println("[WARNING] Initial weather update failed. Will retry later.");
        // Initialize with safe defaults to prevent crash
        currentWeather = CurrentWeather();
        morningForecast = ForecastSummary();
        afternoonForecast = ForecastSummary();
        
        // Set safe default values
        morningForecast.iconCode = "01d"; // Default sunny
        afternoonForecast.iconCode = "01d"; // Default sunny
    }
    
    return false;
}

void WeatherService::parseCurrentWeather(const JsonObject& current) {
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

void WeatherService::parseDailyForecast(const JsonArray& daily) {
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

void WeatherService::parseHourlyForecast(const JsonArray& hourly) {
    // Get the minimum of the array size and our storage capacity
    int count = min((size_t)hourly.size(), (size_t)MAX_HOURLY_FORECASTS);
    hourlyForecastCount = count;
    
    Serial.printf("[INFO] Parsing %d hourly forecasts\n", count);
    
    for (int i = 0; i < count; i++) {
        // Reset to defaults
        hourlyForecasts[i] = HourlyForecast();
        
        JsonObject forecast = hourly[i];
        
        // Parse main fields
        hourlyForecasts[i].dt = forecast["dt"].as<long>();
        hourlyForecasts[i].temp = forecast["temp"].as<float>();
        hourlyForecasts[i].feels_like = forecast["feels_like"].as<float>();
        hourlyForecasts[i].pressure = forecast["pressure"].as<int>();
        hourlyForecasts[i].humidity = forecast["humidity"].as<int>();
        hourlyForecasts[i].dew_point = forecast["dew_point"].as<float>();
        hourlyForecasts[i].clouds = forecast["clouds"].as<int>();
        hourlyForecasts[i].uvi = forecast["uvi"].as<float>();
        hourlyForecasts[i].visibility = forecast["visibility"].as<int>();
        hourlyForecasts[i].wind_speed = forecast["wind_speed"].as<float>();
        hourlyForecasts[i].wind_gust = forecast["wind_gust"].as<float>();
        hourlyForecasts[i].wind_deg = forecast["wind_deg"].as<int>();
        hourlyForecasts[i].pop = forecast["pop"].as<float>();
        
        // Parse optional precipitation
        if (forecast.containsKey("rain") && forecast["rain"].containsKey("1h")) {
            hourlyForecasts[i].rain_1h = forecast["rain"]["1h"].as<float>();
        }
        
        if (forecast.containsKey("snow") && forecast["snow"].containsKey("1h")) {
            hourlyForecasts[i].snow_1h = forecast["snow"]["1h"].as<float>();
        }
        
        // Parse weather condition
        if (forecast.containsKey("weather") && forecast["weather"].size() > 0) {
            JsonObject weather = forecast["weather"][0];
            hourlyForecasts[i].weather_id = weather["id"].as<int>();
            hourlyForecasts[i].weather_main = weather["main"].as<String>();
            hourlyForecasts[i].weather_description = weather["description"].as<String>();
            hourlyForecasts[i].weather_icon = weather["icon"].as<String>();
        }
        
        // Convert timestamp to local time (for debugging)
        time_t timestamp = hourlyForecasts[i].dt;
        struct tm timeinfo;
        localtime_r(&timestamp, &timeinfo);
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        
        Serial.printf("[INFO] Hourly forecast %d: %s, %.1f°C, rain prob: %.0f%%, %s\n", 
                     i, timeStr, hourlyForecasts[i].temp, 
                     hourlyForecasts[i].pop * 100, 
                     hourlyForecasts[i].weather_description.c_str());
    }
}

String WeatherService::getMostFrequentIcon(const HourlyForecast* forecasts, int count) {
    if (count <= 0) return "01d"; // Default icon if no forecasts
    
    // Count occurrences of each icon
    struct IconCount {
        String icon;
        int count = 0;
    };
    
    // Use a simple array since we have a small number of possible icons
    const int MAX_ICONS = 20; // More than enough for all possible OpenWeatherMap icons
    IconCount iconCounts[MAX_ICONS];
    int uniqueIconCount = 0;
    
    for (int i = 0; i < count; i++) {
        String currentIcon = forecasts[i].weather_icon;
        if (currentIcon.isEmpty()) continue;
        
        // Check if this icon is already in our array
        bool found = false;
        for (int j = 0; j < uniqueIconCount; j++) {
            if (iconCounts[j].icon == currentIcon) {
                iconCounts[j].count++;
                found = true;
                break;
            }
        }
        
        // If not found, add it
        if (!found && uniqueIconCount < MAX_ICONS) {
            iconCounts[uniqueIconCount].icon = currentIcon;
            iconCounts[uniqueIconCount].count = 1;
            uniqueIconCount++;
        }
    }
    
    // Find the icon with the highest count
    String mostFrequentIcon = "01d"; // Default
    int maxCount = 0;
    
    for (int i = 0; i < uniqueIconCount; i++) {
        if (iconCounts[i].count > maxCount) {
            maxCount = iconCounts[i].count;
            mostFrequentIcon = iconCounts[i].icon;
        }
    }
    
    return mostFrequentIcon;
}

void WeatherService::calculateDailyForecasts() {
    Serial.println("[INFO] Calculating daily forecasts from hourly data");
    Serial.printf("[DEBUG] Starting with %d hourly forecast entries\n", hourlyForecastCount);
    
    // Reset the morning and afternoon forecasts
    morningForecast = ForecastSummary();
    afternoonForecast = ForecastSummary();
    
    // Safety check - if we have no hourly forecasts, set defaults and return early
    if (hourlyForecastCount <= 0) {
        Serial.println("[WARNING] No hourly forecasts available for daily calculation");
        
        // Set safe defaults for morning forecast
        morningForecast.avgTemp = 0;
        morningForecast.avgPop = 0;
        morningForecast.iconCode = "01d"; // Default sunny
        
        // Set safe defaults for afternoon forecast
        afternoonForecast.avgTemp = 0;
        afternoonForecast.avgPop = 0;
        afternoonForecast.iconCode = "01d"; // Default sunny
        
        return;
    }
        
    // Get the current time to determine which forecasts to use
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int currentHour = timeinfo.tm_hour;
    
    // Calculate timestamps for today's midnight, noon, and tomorrow's midnight
    struct tm todayMidnight = timeinfo;
    todayMidnight.tm_hour = 0;
    todayMidnight.tm_min = 0;
    todayMidnight.tm_sec = 0;
    time_t todayMidnightTime = mktime(&todayMidnight);
    
    time_t todayNoon = todayMidnightTime + 12 * 3600; // Add 12 hours for noon
    time_t tomorrowMidnight = todayMidnightTime + 24 * 3600; // Add 24 hours for tomorrow midnight
    
    // Format and print the important timestamps for debugging
    char nowStr[20], todayMidnightStr[20], todayNoonStr[20], tomorrowMidnightStr[20];
    strftime(nowStr, sizeof(nowStr), "%H:%M %d.%m.%Y", &timeinfo);
    
    struct tm tmpTime;
    
    localtime_r(&todayMidnightTime, &tmpTime);
    strftime(todayMidnightStr, sizeof(todayMidnightStr), "%H:%M %d.%m.%Y", &tmpTime);
    
    localtime_r(&todayNoon, &tmpTime);
    strftime(todayNoonStr, sizeof(todayNoonStr), "%H:%M %d.%m.%Y", &tmpTime);
    
    localtime_r(&tomorrowMidnight, &tmpTime);
    strftime(tomorrowMidnightStr, sizeof(tomorrowMidnightStr), "%H:%M %d.%m.%Y", &tmpTime);
    
    Serial.printf("[DEBUG] Time references - Now: %s, Today midnight: %s, Today noon: %s, Tomorrow midnight: %s\n", 
                  nowStr, todayMidnightStr, todayNoonStr, tomorrowMidnightStr);
    
    // Allocate arrays for hourly forecasts with safety checks
    HourlyForecast* morningForecasts = nullptr;
    HourlyForecast* afternoonForecasts = nullptr;
    int morningCount = 0;
    int afternoonCount = 0;
    
    // Only allocate memory if we have forecast data
    if (hourlyForecastCount > 0) {
        morningForecasts = new HourlyForecast[hourlyForecastCount];
        afternoonForecasts = new HourlyForecast[hourlyForecastCount];
    } else {
        Serial.println("[WARNING] No hourly forecast data available");
        return; // Exit early if no data
    }
    
    // Today's day of month
    int todayDay = timeinfo.tm_mday;
    
    
    if (currentHour < 12) {
        // Time is between 00:00 and 12:00
        Serial.println("[INFO] Morning mode: Using today's morning and afternoon");
        
        // Morning forecast: Today from now until noon
        Serial.println("[DEBUG] Collecting morning forecasts (today from now to noon):");
        for (int i = 0; i < hourlyForecastCount; i++) {
            // Print timestamp info to debug format
            time_t timestamp = hourlyForecasts[i].dt;
            struct tm forecastTime;
            localtime_r(&timestamp, &forecastTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &forecastTime);
            
            // Check if this forecast belongs in the morning group
            bool inRange = (hourlyForecasts[i].dt >= now && hourlyForecasts[i].dt < todayNoon);
            if (inRange) {
                morningForecasts[morningCount++] = hourlyForecasts[i];
                Serial.printf("[DEBUG] ✓ Added to morning: %s (%.1f°C, PoP: %.0f%%)\n", 
                             timeStr, hourlyForecasts[i].temp, hourlyForecasts[i].pop * 100);
            } else {
                Serial.printf("[DEBUG] ✗ Not in morning range: %s\n", timeStr);
            }
        }
        Serial.printf("[DEBUG] Collected %d morning forecasts\n", morningCount);
        
        // Afternoon forecast: Today from noon until midnight
        Serial.println("[DEBUG] Collecting afternoon forecasts (today from noon to midnight):");
        for (int i = 0; i < hourlyForecastCount; i++) {
            // Print timestamp info to debug format
            time_t timestamp = hourlyForecasts[i].dt;
            struct tm forecastTime;
            localtime_r(&timestamp, &forecastTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &forecastTime);
            
            // Check if this forecast belongs in the afternoon group
            bool inRange = (hourlyForecasts[i].dt >= todayNoon && hourlyForecasts[i].dt < tomorrowMidnight);
            if (inRange) {
                afternoonForecasts[afternoonCount++] = hourlyForecasts[i];
                Serial.printf("[DEBUG] ✓ Added to afternoon: %s (%.1f°C, PoP: %.0f%%)\n", 
                             timeStr, hourlyForecasts[i].temp, hourlyForecasts[i].pop * 100);
            } else {
                Serial.printf("[DEBUG] ✗ Not in afternoon range: %s\n", timeStr);
            }
        }
        Serial.printf("[DEBUG] Collected %d afternoon forecasts\n", afternoonCount);
    } else {
        // Time is between 12:01 and 23:59
        Serial.println("[INFO] Afternoon mode: Using today's afternoon and tomorrow's morning");
        
        // Afternoon forecast: Today from now until midnight
        Serial.println("[DEBUG] Collecting afternoon forecasts (today from now to midnight):");
        for (int i = 0; i < hourlyForecastCount; i++) {
            // Print timestamp info to debug format
            time_t timestamp = hourlyForecasts[i].dt;
            struct tm forecastTime;
            localtime_r(&timestamp, &forecastTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &forecastTime);
            
            // Check if this forecast belongs in the afternoon group
            bool inRange = (hourlyForecasts[i].dt >= now && hourlyForecasts[i].dt < tomorrowMidnight);
            if (inRange) {
                afternoonForecasts[afternoonCount++] = hourlyForecasts[i];
                Serial.printf("[DEBUG] ✓ Added to afternoon: %s (%.1f°C, PoP: %.0f%%)\n", 
                             timeStr, hourlyForecasts[i].temp, hourlyForecasts[i].pop * 100);
            } else {
                Serial.printf("[DEBUG] ✗ Not in afternoon range: %s\n", timeStr);
            }
        }
        Serial.printf("[DEBUG] Collected %d afternoon forecasts\n", afternoonCount);
        
        // Morning forecast: Tomorrow from midnight until noon
        time_t tomorrowNoon = tomorrowMidnight + 12 * 3600; // Add 12 hours for noon
        Serial.println("[DEBUG] Collecting morning forecasts (tomorrow from midnight to noon):");
        for (int i = 0; i < hourlyForecastCount; i++) {
            // Print timestamp info to debug format
            time_t timestamp = hourlyForecasts[i].dt;
            struct tm forecastTime;
            localtime_r(&timestamp, &forecastTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &forecastTime);
            
            // Check if this forecast belongs in the morning group
            bool inRange = (hourlyForecasts[i].dt >= tomorrowMidnight && hourlyForecasts[i].dt < tomorrowNoon);
            if (inRange) {
                morningForecasts[morningCount++] = hourlyForecasts[i];
                Serial.printf("[DEBUG] ✓ Added to morning: %s (%.1f°C, PoP: %.0f%%)\n", 
                             timeStr, hourlyForecasts[i].temp, hourlyForecasts[i].pop * 100);
            } else {
                Serial.printf("[DEBUG] ✗ Not in morning range: %s\n", timeStr);
            }
        }
        Serial.printf("[DEBUG] Collected %d morning forecasts\n", morningCount);
    }
    
    // Calculate morning forecast summary
    Serial.println("[DEBUG] Calculating morning forecast summary");
    if (morningCount > 0 && morningForecasts != nullptr) {
        float totalTemp = 0;
        float totalPop = 0;
        
        Serial.println("[DEBUG] Morning forecast inputs:");
        for (int i = 0; i < morningCount; i++) {
            totalTemp += morningForecasts[i].temp;
            totalPop += morningForecasts[i].pop;
            
            // Convert timestamp to readable format
            time_t timestamp = morningForecasts[i].dt;
            struct tm forecastTime;
            localtime_r(&timestamp, &forecastTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &forecastTime);
            
            Serial.printf("[DEBUG] Morning input #%d: %s, %.1f°C, %.0f%%, icon=%s\n",
                        i, timeStr, morningForecasts[i].temp, 
                        morningForecasts[i].pop * 100, morningForecasts[i].weather_icon.c_str());
        }
        
        morningForecast.avgTemp = totalTemp / morningCount;
        morningForecast.avgPop = totalPop / morningCount;
        morningForecast.iconCode = getMostFrequentIcon(morningForecasts, morningCount);
        
        Serial.printf("[INFO] Morning forecast summary: Avg Temp: %.1f°C, Avg PoP: %.0f%%, Icon: %s (using %d hourly samples)\n", 
                     morningForecast.avgTemp, morningForecast.avgPop * 100, 
                     morningForecast.iconCode.c_str(), morningCount);
    } else {
        Serial.println("[WARNING] No hourly data available for morning forecast");
        morningForecast.avgTemp = 0;
        morningForecast.avgPop = 0;
        morningForecast.iconCode = "01d"; // Default sunny
    }
    
    // Calculate afternoon forecast summary
    Serial.println("[DEBUG] Calculating afternoon forecast summary");
    if (afternoonCount > 0 && afternoonForecasts != nullptr) {
        float totalTemp = 0;
        float totalPop = 0;
        
        Serial.println("[DEBUG] Afternoon forecast inputs:");
        for (int i = 0; i < afternoonCount; i++) {
            totalTemp += afternoonForecasts[i].temp;
            totalPop += afternoonForecasts[i].pop;
            
            // Convert timestamp to readable format
            time_t timestamp = afternoonForecasts[i].dt;
            struct tm forecastTime;
            localtime_r(&timestamp, &forecastTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.%Y", &forecastTime);
            
            Serial.printf("[DEBUG] Afternoon input #%d: %s, %.1f°C, %.0f%%, icon=%s\n",
                        i, timeStr, afternoonForecasts[i].temp, 
                        afternoonForecasts[i].pop * 100, afternoonForecasts[i].weather_icon.c_str());
        }
        
        afternoonForecast.avgTemp = totalTemp / afternoonCount;
        afternoonForecast.avgPop = totalPop / afternoonCount;
        afternoonForecast.iconCode = getMostFrequentIcon(afternoonForecasts, afternoonCount);
        
        Serial.printf("[INFO] Afternoon forecast summary: Avg Temp: %.1f°C, Avg PoP: %.0f%%, Icon: %s (using %d hourly samples)\n", 
                     afternoonForecast.avgTemp, afternoonForecast.avgPop * 100, 
                     afternoonForecast.iconCode.c_str(), afternoonCount);
    } else {
        Serial.println("[WARNING] No hourly data available for afternoon forecast");
        afternoonForecast.avgTemp = 0;
        afternoonForecast.avgPop = 0;
        afternoonForecast.iconCode = "01d"; // Default sunny
    }
    
    // Cleanup - only delete if they were allocated
    if (morningForecasts != nullptr) {
        delete[] morningForecasts;
        morningForecasts = nullptr;
    }
    
    if (afternoonForecasts != nullptr) {
        delete[] afternoonForecasts;
        afternoonForecasts = nullptr;
    }
}
