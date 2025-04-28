#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <functional>

#include "mcp/MCPBroker.h"
#include "mcp/MCPReferenceProvider.h"
#include "mcp/MCPReferenceSubscriber.h"
#include "rack/framework/mock.h"

// Utility function to simulate VCV Rack's audio thread
void simulateAudioThread(std::shared_ptr<mcp::MCPReferenceSubscriber> subscriber, 
                         std::atomic<bool>& running,
                         int sampleRate,
                         int blockSize) {
    // Set thread as audio thread
    rack::engine::setThreadType(rack::engine::AUDIO_THREAD);
    
    std::cout << "Audio thread started (sample rate: " << sampleRate << ", block size: " << blockSize << ")" << std::endl;
    
    // Calculate block time
    float blockTimeMs = (blockSize * 1000.0f) / sampleRate;
    
    // Buffer for audio processing
    std::unique_ptr<float[]> buffer(new float[blockSize]);
    
    // Process counter
    int processCount = 0;
    
    // Simulation loop
    while (running) {
        // Process audio block
        subscriber->process(buffer.get(), blockSize);
        processCount++;
        
        // Log occasionally
        if (processCount % 100 == 0) {
            std::cout << "Audio thread processed " << processCount 
                      << " blocks (" << (processCount * blockSize) << " samples)" << std::endl;
            
            // Display first few samples
            std::cout << "  Output samples: [";
            for (int i = 0; i < std::min(5, blockSize); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << std::fixed << std::setprecision(3) << buffer[i];
            }
            std::cout << "]" << std::endl;
        }
        
        // Sleep to simulate timing of audio callback
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(blockTimeMs)));
    }
    
    std::cout << "Audio thread completed after processing " << processCount << " blocks" << std::endl;
}

// Display parameter array nicely
std::string formatParameterArray(const std::vector<float>& params) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < params.size(); i++) {
        if (i > 0) ss << ", ";
        ss << std::fixed << std::setprecision(2) << params[i];
    }
    ss << "]";
    return ss.str();
}

// Monitoring thread for subscriber values
void monitorSubscriberThread(std::shared_ptr<mcp::MCPReferenceSubscriber> subscriber, 
                             std::atomic<bool>& running) {
    std::cout << "Monitoring thread started" << std::endl;
    
    float lastParam1 = -1.0f;
    float lastParam2 = -1.0f;
    std::string lastPreset;
    std::vector<float> lastParams;
    int updateCount = 0;
    
    while (running) {
        // Get current values (thread-safe via subscriber methods)
        float param1 = subscriber->getParameter(1);
        float param2 = subscriber->getParameter(2);
        std::string preset = subscriber->getPreset();
        std::vector<float> params = subscriber->getParameterArray();
        
        // Check if anything changed
        bool changed = 
            param1 != lastParam1 ||
            param2 != lastParam2 ||
            preset != lastPreset ||
            params != lastParams;
        
        // Display changes
        if (changed) {
            std::cout << "\n=== Subscriber update #" << ++updateCount << " ===" << std::endl;
            
            if (param1 != lastParam1) {
                std::cout << "Parameter 1: " << std::fixed << std::setprecision(3) << param1 << std::endl;
                lastParam1 = param1;
            }
            
            if (param2 != lastParam2) {
                std::cout << "Parameter 2: " << std::fixed << std::setprecision(3) << param2 << std::endl;
                lastParam2 = param2;
            }
            
            if (preset != lastPreset) {
                std::cout << "Preset: \"" << preset << "\"" << std::endl;
                lastPreset = preset;
            }
            
            if (params != lastParams) {
                std::cout << "Parameters: " << formatParameterArray(params) << std::endl;
                lastParams = params;
            }
        }
        
        // Check every 200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "Monitoring thread completed with " << updateCount << " updates" << std::endl;
}

int main() {
    std::cout << "===== VCV Rack MCP Reference Implementation Example =====" << std::endl;
    std::cout << "This example demonstrates a complete implementation of the" << std::endl;
    std::cout << "Model Context Protocol with thread-safe data passing." << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    try {
        // Get broker instance
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return 1;
        }
        
        std::cout << "MCP Broker obtained (version " << broker->getVersion() << ")" << std::endl;
        
        // Create provider and subscriber
        auto provider = std::make_shared<mcp::MCPReferenceProvider>(1001);
        auto subscriber = std::make_shared<mcp::MCPReferenceSubscriber>(2001);
        
        std::cout << "Created provider (ID: " << provider->getId() << ") and subscriber (ID: " << subscriber->getId() << ")" << std::endl;
        
        // Initialize the modules (similar to adding to VCV Rack)
        std::cout << "\nAdding modules to the rack..." << std::endl;
        provider->onAdd();
        subscriber->onAdd();
        
        // Start threads for simulation
        std::atomic<bool> running(true);
        
        // Start audio thread with explicit sample rate and block size
        const int SAMPLE_RATE = 44100;
        const int BLOCK_SIZE = 256;
        std::thread audioThread(simulateAudioThread, subscriber, std::ref(running), 
                               SAMPLE_RATE, BLOCK_SIZE);
        
        // Start monitoring thread
        std::thread monitorThread(monitorSubscriberThread, subscriber, std::ref(running));
        
        // Start periodic publishing
        std::cout << "\nStarting periodic publishing..." << std::endl;
        provider->startPeriodicPublishing(500); // Publish every 500ms
        
        // Run for 10 seconds
        std::cout << "\nSimulation running for 10 seconds..." << std::endl;
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "." << std::flush;
        }
        std::cout << std::endl;
        
        // Stop all threads
        std::cout << "\nStopping simulation..." << std::endl;
        running = false;
        
        // Wait for threads to finish
        if (audioThread.joinable()) {
            audioThread.join();
        }
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
        
        // Stop periodic publishing
        provider->stopPeriodicPublishing();
        
        // Remove modules (similar to removing from VCV Rack)
        std::cout << "\nRemoving modules from the rack..." << std::endl;
        subscriber->onRemove();
        provider->onRemove();
        
        std::cout << "\nExample completed successfully!" << std::endl;
        
        // Clear strong references in proper order
        subscriber.reset();
        provider.reset();
        
        // Release the broker instance last
        mcp::MCPBroker::releaseInstance();
        broker.reset();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 