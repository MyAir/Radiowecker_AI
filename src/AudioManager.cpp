#include "AudioManager.h"
#include "ConfigManager.h"
#include <SD.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Initialize static member
AudioManager* AudioManager::instance = nullptr;

// Audio callbacks
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
    const char *ptr = reinterpret_cast<const char *>(cbData);
    (void) isUnicode; // Punt this ball for now
    if (strcmp(type, "SS") == 0) {
        Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, type, string);
    } else {
        // Ignore other metadata types for now
    }
}

// Constructor is defined as default in the header file

AudioManager::~AudioManager() {
    cleanup();
}

void AudioManager::begin() {
    // Initialize I2S audio output
    audioOutput = new AudioOutputI2S();
    audioOutput->SetGain(currentVolume / 100.0);
    
    // Use regular SD card which was already initialized in main.cpp
    // SD_MMC replaced with SD to fix initialization errors
    if (!SD.begin(SD_CS)) {
        Serial.println("SD initialization check failed in AudioManager!");
        // Continue anyway as SD might already be initialized
    }
}

void AudioManager::loop() {
    if (audioGenerator && audioGenerator->isRunning()) {
        if (!audioGenerator->loop()) {
            // Playback finished or error occurred
            audioGenerator->stop();
            cleanup();
            notifyPlaybackState(false);
        }
    }
}

bool AudioManager::playStream(const char* url) {
    stop();
    
    Serial.printf("Connecting to stream: %s\n", url);
    
    // Create stream source
    if (String(url).startsWith("http")) {
        // HTTP stream
        auto* stream = new AudioFileSourceHTTPStream();
        if (!stream->open(url)) {
            Serial.println("Failed to open HTTP stream");
            delete stream;
            return false;
        }
        
        // Create buffered source
        bufferedSource = new AudioFileSourceBuffer(stream, 1024 * 8);
        fileSource = bufferedSource;
    } else {
        // Local file
        fileSource = new AudioFileSourceSD(url);
    }
    
    // Create MP3 decoder
    audioGenerator = new AudioGeneratorMP3();
    if (!audioGenerator->begin(fileSource, audioOutput)) {
        Serial.println("Failed to start MP3 decoder");
        cleanup();
        return false;
    }
    
    isStreaming = true;
    notifyPlaybackState(true);
    return true;
}

bool AudioManager::playFile(const char* filename) {
    stop();
    
    if (!filename) return false;
    
    // Check if file exists on SD card
    if (!SD.exists(filename)) {
        Serial.printf("File not found: %s\n", filename);
        return false;
    }
    
    // Create file source directly from filename
    fileSource = new AudioFileSourceSD(filename);
    
    if (!fileSource->isOpen()) {
        Serial.printf("Failed to open file: %s\n", filename);
        cleanup();
        return false;
    }
    
    // Create buffered source
    bufferedSource = new AudioFileSourceBuffer(fileSource, 8 * 1024);
    
    // Create MP3 decoder
    audioGenerator = new AudioGeneratorMP3();
    if (!audioGenerator->begin(bufferedSource, audioOutput)) {
        Serial.println("Failed to start MP3 decoder");
        cleanup();
        return false;
    }
    
    isStreaming = false;
    notifyPlaybackState(true);
    return true;
}

void AudioManager::stop() {
    if (audioGenerator) {
        if (audioGenerator->isRunning()) {
            audioGenerator->stop();
            notifyPlaybackState(false);
        }
    }
    cleanup();
}

void AudioManager::setVolume(uint8_t volume) {
    currentVolume = constrain(volume, 0, 100);
    if (audioOutput) {
        audioOutput->SetGain(currentVolume / 100.0);
    }
}

void AudioManager::cleanup() {
    if (audioGenerator) {
        if (audioGenerator->isRunning()) {
            audioGenerator->stop();
        }
        delete audioGenerator;
        audioGenerator = nullptr;
    }
    
    if (bufferedSource) {
        delete bufferedSource;
        bufferedSource = nullptr;
    }
    
    if (fileSource) {
        delete fileSource;
        fileSource = nullptr;
    }
    
    isStreaming = false;
}

void AudioManager::notifyPlaybackState(bool isPlaying) {
    lastStateChange = millis();
    if (playbackStateCallback) {
        playbackStateCallback(isPlaying);
    }
}
