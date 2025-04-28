#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <functional>
#include <map>

#include "mcp/MCPBroker.h"
#include "mcp/MCPReferenceProvider.h"
#include "mcp/MCPReferenceSubscriber.h"
#include "rack/framework/mock.h"

// Utility function to simulate audio processing
void audioThreadFunc(std::shared_ptr<rack::Module> module, bool& running, int sampleRate, int blockSize) {
    // Set thread type for proper identification
    rack::engine::setThreadType(rack::engine::AUDIO_THREAD);
    
    std::cout << "Audio thread started with sample rate " << sampleRate << " Hz, block size " << blockSize << std::endl;
    
    // Buffer for audio processing
    float* buffer = new float[blockSize];
    
    // Calculate timing
    double blockTimeMs = (blockSize * 1000.0) / sampleRate;
    std::cout << "Audio block time: " << std::fixed << std::setprecision(2) << blockTimeMs << " ms" << std::endl;
    
    // Process audio at regular intervals
    int blockCount = 0;
    while (running) {
        // Process audio
        module->process(buffer, blockSize);
        
        // Print audio output occasionally
        if (blockCount % 100 == 0) {
            std::cout << "Audio output (first 5 samples): ";
            for (int i = 0; i < std::min(5, blockSize); ++i) {
                std::cout << std::fixed << std::setprecision(3) << buffer[i] << " ";
            }
            std::cout << std::endl;
        }
        
        blockCount++;
        
        // Sleep to simulate audio callback timing
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(blockTimeMs)));
    }
    
    // Cleanup
    delete[] buffer;
    
    std::cout << "Audio thread stopped after processing " << blockCount << " blocks" << std::endl;
}

// Format parameter array for display
std::string formatParameterArray(const std::vector<float>& params) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << std::fixed << std::setprecision(2) << params[i];
    }
    oss << "]";
    return oss.str();
}

// Monitor module state in a separate thread
void monitorThread(std::shared_ptr<mcp::MCPReferenceSubscriber> subscriber, bool& running) {
    std::cout << "\nStarting monitoring thread..." << std::endl;
    
    int updateCount = 0;
    std::map<std::string, std::string> lastValues;
    
    while (running) {
        // Gather current values
        std::map<std::string, std::string> currentValues;
        currentValues["Parameter 1"] = std::to_string(subscriber->getParameter(1));
        currentValues["Parameter 2"] = std::to_string(subscriber->getParameter(2));
        currentValues["Preset"] = subscriber->getPreset();
        currentValues["Parameters"] = formatParameterArray(subscriber->getParameterArray());
        
        // Check for changes
        bool hasChanges = false;
        for (const auto& pair : currentValues) {
            if (lastValues.count(pair.first) == 0 || lastValues[pair.first] != pair.second) {
                hasChanges = true;
                break;
            }
        }
        
        // Display if changed
        if (hasChanges) {
            std::cout << "\n===== Update #" << ++updateCount << " =====" << std::endl;
            for (const auto& pair : currentValues) {
                std::cout << pair.first << ": " << pair.second << std::endl;
            }
            lastValues = currentValues;
        }
        
        // Check every 200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "Monitoring thread stopped after " << updateCount << " updates" << std::endl;
}

int main() {
    std::cout << "MCP Reference Modules Example" << std::endl;
    std::cout << "============================" << std::endl;

    // Scope variables to control lifecycle
    {
        // Get broker instance
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return 1;
        }
        
        std::cout << "MCP Broker obtained (version " << broker->getVersion() << ")" << std::endl;
        
        // Create provider and subscriber with unique IDs
        auto provider = std::make_shared<mcp::MCPReferenceProvider>(1001);
        auto subscriber = std::make_shared<mcp::MCPReferenceSubscriber>(2001);
        
        std::cout << "\nCreated provider (ID: " << provider->getId() << ") and subscriber (ID: " 
                  << subscriber->getId() << ")" << std::endl;
        
        // Simulate adding modules to the rack
        provider->onAdd();
        subscriber->onAdd();
        
        std::cout << "\nAdded both modules to the rack" << std::endl;
        std::cout << "Provider publishes to topics: ";
        for (const auto& topic : provider->getProvidedTopics()) {
            std::cout << topic << " ";
        }
        std::cout << std::endl;
        
        // Start audio thread for the subscriber
        bool audioRunning = true;
        const int SAMPLE_RATE = 44100;
        const int BLOCK_SIZE = 256;
        std::thread audioThread(audioThreadFunc, subscriber, std::ref(audioRunning),
                               SAMPLE_RATE, BLOCK_SIZE);
        
        // Start monitoring thread to display value changes
        bool monitorRunning = true;
        std::thread monitoringThread(monitorThread, subscriber, std::ref(monitorRunning));
        
        // Run for specified duration
        const int RUN_DURATION_SECONDS = 15;
        std::cout << "\nRunning for " << RUN_DURATION_SECONDS << " seconds..." << std::endl;
        
        // Progress bar for runtime
        for (int i = 0; i < RUN_DURATION_SECONDS; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "." << std::flush;
        }
        std::cout << std::endl;
        
        // Stop monitoring thread
        monitorRunning = false;
        if (monitoringThread.joinable()) {
            monitoringThread.join();
        }
        
        // Simulate removing modules from the rack
        subscriber->onRemove();
        provider->onRemove();
        
        // Stop audio thread
        audioRunning = false;
        if (audioThread.joinable()) {
            audioThread.join();
        }
        
        std::cout << "\nRemoved both modules from the rack" << std::endl;
        
        // Display final statistics
        std::cout << "\nExample completed successfully" << std::endl;
        
        // Ensure all shared_ptr objects are properly released in proper order
        provider.reset();
        subscriber.reset();
        broker.reset();
    }
    
    // Ensure broker is properly shut down
    mcp::shutdownMCPBroker();
    
    return 0;
} 