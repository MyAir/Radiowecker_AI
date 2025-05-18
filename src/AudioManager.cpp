#include "AudioManager.h"
#include "ConfigManager.h"
#include <SD_MMC.h>

// Global instance
AudioManager& audioManager = AudioManager::getInstance();

AudioManager::AudioManager() {
    // Initialize with default values
}

AudioManager::~AudioManager() {
    cleanup();
}

void AudioManager::begin() {
    // Initialize I2S audio output
    audioOutput = new AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S, 10);
    audioOutput->SetOutputModeMono(false);
    audioOutput->SetGain(currentVolume / 100.0);
    
    // Initialize SD_MMC if not already done
    if (!SD_MMC.begin("/sdcard", true, true)) {
        Serial.println("SD_MMC initialization failed!");
        // Continue without SD card support
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
    
    try {
        // Create stream source
        auto* stream = new AudioFileSourceICYStream(url);
        stream->RegisterMetadata(
            [](void* cbData, const char* type, bool isUnicode, const char* str) {
                const char* ptr = reinterpret_cast<const char*>(cbData);
                Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, type, str);
            }, (void*)"ICY");

        // Create buffered source
        bufferedSource = new AudioFileSourceBuffer(stream, bufferSize);
        bufferedSource->registerPreFillCallback(
            [](void* userdata, AudioFileSourceBuffer::PreFillStatus status, int32_t bytesAvailable, uint32_t bytesAdded) {
                auto* mgr = static_cast<AudioManager*>(userdata);
                if (status == AudioFileSourceBuffer::PreFillStatus::FILLING) {
                    float percentFilled = (bytesAvailable * 100.0) / mgr->bufferSize;
                    if (percentFilled >= mgr->preBufferPercent) {
                        return true;  // Start playback
                    }
                }
                return false;  // Continue buffering
            }, this);
        
        // Create MP3 decoder
        audioGenerator = new AudioGeneratorMP3();
        
        if (audioGenerator->begin(bufferedSource, audioOutput)) {
            isStreaming = true;
            notifyPlaybackState(true);
            return true;
        }
    } catch (...) {
        Serial.println("Exception occurred while starting stream");
    }
    
    cleanup();
    return false;
}

bool AudioManager::playFile(const char* filename) {
    stop();
    
    if (!SD_MMC.exists(filename)) {
        Serial.printf("File not found: %s\n", filename);
        return false;
    }
    
    try {
        fileSource = new AudioFileSourceSD(filename);
        audioGenerator = new AudioGeneratorMP3();
        
        if (audioGenerator->begin(fileSource, audioOutput)) {
            isStreaming = false;
            notifyPlaybackState(true);
            return true;
        }
    } catch (...) {
        Serial.println("Exception occurred while playing file");
    }
    
    cleanup();
    return false;
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
