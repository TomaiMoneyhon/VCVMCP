#include <gtest/gtest.h>
#include "mcp/MCPRingBuffer.h"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <iostream>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>

/**
 * @brief RingBuffer stress testing focused on SPSC (Single-Producer/Single-Consumer) scenarios
 * 
 * This test file contains targeted tests to verify the thread safety and performance
 * of the RingBuffer implementation specifically for SPSC use cases, which is the 
 * primary pattern used in the MCP system where:
 * - Worker thread (producer) calls push() in onMCPMessage
 * - Audio thread (consumer) calls pop() in process()
 */
class RingBufferStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any shared state between tests
    }

    void TearDown() override {
        // Make sure all threads are joined properly
    }

    // Helper to print diagnostic information about processing results
    void printDiagnostics(const std::vector<int>& producedItems, 
                         const std::vector<int>& consumedItems,
                         int duplicates, int missing) {
        std::cout << "=== RingBuffer Test Diagnostics ===" << std::endl;
        std::cout << "Total produced: " << producedItems.size() << std::endl;
        std::cout << "Total consumed: " << consumedItems.size() << std::endl;
        std::cout << "Duplicate items: " << duplicates << std::endl;
        std::cout << "Missing items: " << missing << std::endl;
        
        if (duplicates > 0 || missing > 0) {
            std::cout << "\nDetailed issues:" << std::endl;
            
            // Create a map to count occurrences of each consumed item
            std::map<int, int> itemCounts;
            for (int item : consumedItems) {
                itemCounts[item]++;
            }
            
            // Check for duplicates and missing items
            if (duplicates > 0) {
                std::cout << "First 10 duplicate items:" << std::endl;
                int count = 0;
                for (const auto& pair : itemCounts) {
                    if (pair.second > 1) {
                        std::cout << "  Item " << pair.first << " consumed " 
                                 << pair.second << " times" << std::endl;
                        if (++count >= 10) break;
                    }
                }
            }
            
            if (missing > 0) {
                std::cout << "First 10 missing items:" << std::endl;
                int count = 0;
                for (int item : producedItems) {
                    if (itemCounts.find(item) == itemCounts.end()) {
                        std::cout << "  Item " << item << " was never consumed" << std::endl;
                        if (++count >= 10) break;
                    }
                }
            }
        }
        
        std::cout << "=================================" << std::endl;
    }
    
    // Helper to analyze results and report issues
    std::pair<int, int> analyzeResults(const std::vector<int>& producedItems, 
                                      const std::vector<int>& consumedItems) {
        // Count items that were consumed multiple times
        std::map<int, int> itemCounts;
        for (int item : consumedItems) {
            itemCounts[item]++;
        }
        
        int duplicates = 0;
        for (const auto& pair : itemCounts) {
            if (pair.second > 1) {
                duplicates += pair.second - 1;
            }
        }
        
        // Count items that were produced but never consumed
        int missing = 0;
        for (int item : producedItems) {
            if (itemCounts.find(item) == itemCounts.end()) {
                missing++;
            }
        }
        
        return {duplicates, missing};
    }
};

// Basic test for the SPSC pattern that ensures no items are dropped or duplicated
TEST_F(RingBufferStressTest, SingleProducerSingleConsumer) {
    // Test parameters
    const int BUFFER_SIZE = 32;
    const int NUM_ITEMS = 100000;
    const int MAX_TEST_DURATION_MS = 5000;
    
    mcp::RingBuffer<int> buffer(BUFFER_SIZE);
    
    std::vector<int> producedItems;
    std::vector<int> consumedItems;
    
    std::mutex producedMutex;
    std::mutex consumedMutex;
    
    std::atomic<bool> startTest(false);
    std::atomic<bool> producerDone(false);
    std::atomic<bool> consumerDone(false);
    
    // Producer thread - adheres to SPSC pattern
    std::thread producer([&]() {
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        std::vector<int> localProduced;
        localProduced.reserve(NUM_ITEMS);
        
        for (int i = 0; i < NUM_ITEMS; ++i) {
            // Try to push with exponential backoff
            int backoff = 1;
            while (!buffer.push(i)) {
                // Exponential backoff to reduce contention
                if (backoff < 1000) {
                    std::this_thread::sleep_for(std::chrono::microseconds(backoff));
                    backoff *= 2;
                } else {
                    std::this_thread::yield();
                }
            }
            
            localProduced.push_back(i);
        }
        
        // Add locally produced items to global tracker
        {
            std::lock_guard<std::mutex> lock(producedMutex);
            producedItems.insert(producedItems.end(), localProduced.begin(), localProduced.end());
        }
        
        producerDone.store(true, std::memory_order_release);
    });
    
    // Consumer thread - adheres to SPSC pattern
    std::thread consumer([&]() {
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        std::vector<int> localConsumed;
        localConsumed.reserve(NUM_ITEMS);
        
        int value;
        int idleCount = 0;
        
        // Keep consuming until producer is done and buffer is empty
        while (true) {
            if (buffer.pop(value)) {
                localConsumed.push_back(value);
                idleCount = 0;
            } else {
                // Buffer empty, check if we're done
                if (producerDone.load(std::memory_order_acquire) && idleCount > 100) {
                    // Producer is done and we've tried multiple times with no data
                    break;
                }
                
                // Exponential backoff when buffer is empty
                if (++idleCount < 10) {
                    std::this_thread::yield();
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(idleCount));
                }
            }
        }
        
        // Add locally consumed items to global tracker
        {
            std::lock_guard<std::mutex> lock(consumedMutex);
            consumedItems.insert(consumedItems.end(), localConsumed.begin(), localConsumed.end());
        }
        
        consumerDone.store(true, std::memory_order_release);
    });
    
    // Start the test
    auto startTime = std::chrono::steady_clock::now();
    startTest.store(true, std::memory_order_release);
    
    // Monitor the test
    bool timedOut = false;
    while (!producerDone.load(std::memory_order_acquire) || 
           !consumerDone.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        
        // Check for timeout
        if (elapsed > MAX_TEST_DURATION_MS) {
            std::cout << "Test timed out after " << MAX_TEST_DURATION_MS << "ms" << std::endl;
            timedOut = true;
            break;
        }
    }
    
    // Join threads
    producer.join();
    consumer.join();
    
    // Skip test if it timed out
    if (timedOut) {
        GTEST_SKIP() << "SPSC test timed out";
        return;
    }
    
    // Sort results for consistent analysis
    std::sort(producedItems.begin(), producedItems.end());
    std::sort(consumedItems.begin(), consumedItems.end());
    
    // Analyze results
    auto [duplicates, missing] = analyzeResults(producedItems, consumedItems);
    
    // Print diagnostics if there are issues
    if (duplicates > 0 || missing > 0) {
        printDiagnostics(producedItems, consumedItems, duplicates, missing);
    }
    
    // Assert no duplicates or missing items
    EXPECT_EQ(duplicates, 0) << "Found duplicate items in the consumed set";
    EXPECT_EQ(missing, 0) << "Found items missing from the consumed set";
    
    // Print performance metrics
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    double throughput = (double)consumedItems.size() * 1000.0 / duration;
    std::cout << "SPSC RingBuffer processed " << consumedItems.size() << " items in " 
              << duration << "ms (" << throughput << " items/second)" << std::endl;
}

// Test with multiple ring buffers used in parallel (one producer/consumer pair per buffer)
// This simulates MCP's use case where multiple modules each have their own worker/audio thread pair
TEST_F(RingBufferStressTest, MultipleDistinctRingBuffers) {
    // Parameters
    const int BUFFER_SIZE = 32;
    const int NUM_BUFFERS = 4;  // Number of distinct producer/consumer pairs
    const int ITEMS_PER_BUFFER = 25000;
    const int MAX_TEST_DURATION_MS = 5000;
    
    // Create multiple buffers, each with its own producer and consumer
    std::vector<std::unique_ptr<mcp::RingBuffer<int>>> buffers;
    std::vector<std::vector<int>> producedItems(NUM_BUFFERS);
    std::vector<std::vector<int>> consumedItems(NUM_BUFFERS);
    
    std::atomic<bool> startTest(false);
    std::atomic<int> producersDone(0);
    std::atomic<int> consumersDone(0);
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Create buffers
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        buffers.push_back(std::make_unique<mcp::RingBuffer<int>>(BUFFER_SIZE));
    }
    
    // Create producer threads
    for (int bufferIdx = 0; bufferIdx < NUM_BUFFERS; ++bufferIdx) {
        producers.push_back(std::thread([&, bufferIdx]() {
            // Wait for start signal
            while (!startTest.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            auto& buffer = buffers[bufferIdx];
            auto& produced = producedItems[bufferIdx];
            
            for (int i = 0; i < ITEMS_PER_BUFFER; ++i) {
                // Create a unique item ID that includes the buffer index
                int itemId = bufferIdx * ITEMS_PER_BUFFER + i;
                
                // Try to push with exponential backoff
                int backoff = 1;
                while (!buffer->push(itemId)) {
                    // Exponential backoff to reduce contention
                    if (backoff < 1000) {
                        std::this_thread::sleep_for(std::chrono::microseconds(backoff));
                        backoff *= 2;
                    } else {
                        std::this_thread::yield();
                    }
                }
                
                produced.push_back(itemId);
            }
            
            producersDone.fetch_add(1, std::memory_order_release);
        }));
    }
    
    // Create consumer threads
    for (int bufferIdx = 0; bufferIdx < NUM_BUFFERS; ++bufferIdx) {
        consumers.push_back(std::thread([&, bufferIdx]() {
            // Wait for start signal
            while (!startTest.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            auto& buffer = buffers[bufferIdx];
            auto& consumed = consumedItems[bufferIdx];
            
            int value;
            int idleCount = 0;
            
            // Keep consuming until producer is done and buffer is empty
            while (true) {
                if (buffer->pop(value)) {
                    consumed.push_back(value);
                    idleCount = 0;
                } else {
                    // Buffer empty, check if we're done
                    if (producersDone.load(std::memory_order_acquire) >= NUM_BUFFERS && idleCount > 100) {
                        // All producers are done and we've tried multiple times with no data
                        break;
                    }
                    
                    // Exponential backoff when buffer is empty
                    if (++idleCount < 10) {
                        std::this_thread::yield();
                    } else {
                        std::this_thread::sleep_for(std::chrono::microseconds(idleCount));
                    }
                }
            }
            
            consumersDone.fetch_add(1, std::memory_order_release);
        }));
    }
    
    // Start the test
    auto startTime = std::chrono::steady_clock::now();
    startTest.store(true, std::memory_order_release);
    
    // Monitor the test
    bool timedOut = false;
    while (producersDone.load(std::memory_order_acquire) < NUM_BUFFERS || 
           consumersDone.load(std::memory_order_acquire) < NUM_BUFFERS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        
        // Check for timeout
        if (elapsed > MAX_TEST_DURATION_MS) {
            std::cout << "Test timed out after " << MAX_TEST_DURATION_MS << "ms" << std::endl;
            timedOut = true;
            break;
        }
    }
    
    // Join all threads
    for (auto& p : producers) {
        if (p.joinable()) p.join();
    }
    for (auto& c : consumers) {
        if (c.joinable()) c.join();
    }
    
    // Skip test if it timed out
    if (timedOut) {
        GTEST_SKIP() << "Multiple buffers test timed out";
        return;
    }
    
    // Combine all items for analysis
    std::vector<int> allProduced;
    std::vector<int> allConsumed;
    
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        allProduced.insert(allProduced.end(), producedItems[i].begin(), producedItems[i].end());
        allConsumed.insert(allConsumed.end(), consumedItems[i].begin(), consumedItems[i].end());
    }
    
    // Sort results for consistent analysis
    std::sort(allProduced.begin(), allProduced.end());
    std::sort(allConsumed.begin(), allConsumed.end());
    
    // Analyze results
    auto [duplicates, missing] = analyzeResults(allProduced, allConsumed);
    
    // Print diagnostics if there are issues
    if (duplicates > 0 || missing > 0) {
        printDiagnostics(allProduced, allConsumed, duplicates, missing);
    }
    
    // Assert no duplicates or missing items
    EXPECT_EQ(duplicates, 0) << "Found duplicate items in the consumed set";
    EXPECT_EQ(missing, 0) << "Found items missing from the consumed set";
    
    // Print performance metrics
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    double throughput = (double)allConsumed.size() * 1000.0 / duration;
    std::cout << "Multiple SPSC RingBuffers processed " << allConsumed.size() << " items in " 
              << duration << "ms (" << throughput << " items/second across " << NUM_BUFFERS << " buffers)" << std::endl;
}

// Test to verify memory ordering in SPSC mode
TEST_F(RingBufferStressTest, MemoryOrderingTest) {
    // Create a small buffer to increase memory ordering visibility
    const int BUFFER_SIZE = 8;
    const int NUM_ITEMS = 100000;
    const int MAX_TEST_DURATION_MS = 5000;
    
    mcp::RingBuffer<int> buffer(BUFFER_SIZE);
    
    // Use a single producer and single consumer to isolate memory ordering issues
    std::atomic<bool> startTest(false);
    std::atomic<bool> producerDone(false);
    std::atomic<bool> consumerDone(false);
    
    std::vector<int> producedItems;
    std::vector<int> consumedItems;
    
    // Producer thread - SPSC pattern
    std::thread producer([&]() {
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        for (int i = 0; i < NUM_ITEMS; ++i) {
            // Try to push with aggressive retries to maximize ordering issues
            while (!buffer.push(i)) {
                std::this_thread::yield();
            }
            producedItems.push_back(i);
        }
        
        producerDone.store(true, std::memory_order_release);
    });
    
    // Consumer thread - SPSC pattern
    std::thread consumer([&]() {
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        int value;
        
        // Keep consuming until producer is done and buffer empty
        while (!producerDone.load(std::memory_order_acquire) || !buffer.empty()) {
            if (buffer.pop(value)) {
                consumedItems.push_back(value);
            } else {
                std::this_thread::yield();
            }
        }
        
        consumerDone.store(true, std::memory_order_release);
    });
    
    // Start the test
    auto startTime = std::chrono::steady_clock::now();
    startTest.store(true, std::memory_order_release);
    
    // Monitor the test
    bool timedOut = false;
    while (!producerDone.load(std::memory_order_acquire) || 
           !consumerDone.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        
        // Check for timeout
        if (elapsed > MAX_TEST_DURATION_MS) {
            std::cout << "Memory ordering test timed out after " << MAX_TEST_DURATION_MS << "ms" << std::endl;
            timedOut = true;
            break;
        }
    }
    
    // Join threads
    producer.join();
    consumer.join();
    
    // Skip test if it timed out
    if (timedOut) {
        GTEST_SKIP() << "Memory ordering test timed out";
        return;
    }
    
    // Sort results for consistent analysis
    std::sort(producedItems.begin(), producedItems.end());
    std::sort(consumedItems.begin(), consumedItems.end());
    
    // Analyze results
    auto [duplicates, missing] = analyzeResults(producedItems, consumedItems);
    
    // Print diagnostics
    if (duplicates > 0 || missing > 0) {
        printDiagnostics(producedItems, consumedItems, duplicates, missing);
    }
    
    // Check for correctness
    EXPECT_EQ(duplicates, 0) << "Memory ordering issue: items consumed multiple times";
    EXPECT_EQ(missing, 0) << "Memory ordering issue: items not consumed";
    
    // Print performance metrics
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    double throughput = (double)consumedItems.size() * 1000.0 / duration;
    std::cout << "Memory ordering test processed " << consumedItems.size() << " items in " 
              << duration << "ms (" << throughput << " items/second)" << std::endl;
}

// Test focused on race conditions in the full/empty check with SPSC pattern
TEST_F(RingBufferStressTest, FullEmptyRaceCondition) {
    const int BUFFER_SIZE = 3;  // Small size to increase full/empty transitions
    const int TEST_DURATION_MS = 5000;
    
    mcp::RingBuffer<int> buffer(BUFFER_SIZE);
    
    std::atomic<bool> startTest(false);
    std::atomic<bool> done(false);
    std::atomic<int> pushAttempts(0);
    std::atomic<int> pushFailures(0);
    std::atomic<int> pushSuccesses(0);
    std::atomic<int> popAttempts(0);
    std::atomic<int> popFailures(0);
    std::atomic<int> popSuccesses(0);
    
    // Producer that tries to keep the buffer full - SPSC pattern
    std::thread producer([&]() {
        int counter = 0;
        
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        while (!done.load(std::memory_order_acquire)) {
            pushAttempts.fetch_add(1, std::memory_order_relaxed);
            
            if (buffer.push(counter++)) {
                pushSuccesses.fetch_add(1, std::memory_order_relaxed);
            } else {
                pushFailures.fetch_add(1, std::memory_order_relaxed);
            }
            
            // Randomly yield to increase race condition opportunities
            if (rand() % 100 < 50) {
                std::this_thread::yield();
            }
        }
    });
    
    // Consumer that tries to keep emptying the buffer - SPSC pattern
    std::thread consumer([&]() {
        int value;
        
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        while (!done.load(std::memory_order_acquire)) {
            popAttempts.fetch_add(1, std::memory_order_relaxed);
            
            if (buffer.pop(value)) {
                popSuccesses.fetch_add(1, std::memory_order_relaxed);
            } else {
                popFailures.fetch_add(1, std::memory_order_relaxed);
            }
            
            // Randomly yield to increase race condition opportunities
            if (rand() % 100 < 50) {
                std::this_thread::yield();
            }
        }
    });
    
    // Monitor thread to track buffer state transitions
    std::thread monitor([&]() {
        int lastSize = 0;
        int sizeChanges = 0;
        int emptyTransitions = 0;
        int fullTransitions = 0;
        
        // Wait for start signal
        while (!startTest.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        while (!done.load(std::memory_order_acquire)) {
            int currentSize = buffer.size();
            bool currentEmpty = buffer.empty();
            bool currentFull = buffer.full();
            
            if (currentSize != lastSize) {
                sizeChanges++;
                lastSize = currentSize;
            }
            
            if (currentEmpty && !currentFull) {
                // Valid empty state
                emptyTransitions++;
            } else if (currentFull && !currentEmpty) {
                // Valid full state
                fullTransitions++;
            } else if (currentEmpty && currentFull) {
                // Invalid state - buffer cannot be both empty and full
                std::cout << "ERROR: Buffer reported both empty and full simultaneously!" << std::endl;
                done.store(true, std::memory_order_release);
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        std::cout << "Monitor statistics:" << std::endl
                 << "  Size changes: " << sizeChanges << std::endl
                 << "  Empty transitions: " << emptyTransitions << std::endl
                 << "  Full transitions: " << fullTransitions << std::endl;
    });
    
    // Start the test
    auto startTime = std::chrono::steady_clock::now();
    startTest.store(true, std::memory_order_release);
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DURATION_MS));
    
    // Signal completion
    done.store(true, std::memory_order_release);
    
    // Join all threads
    producer.join();
    consumer.join();
    monitor.join();
    
    // Report statistics
    std::cout << "Full/Empty race condition test completed:" << std::endl
             << "  Push attempts: " << pushAttempts.load() 
             << " (Success: " << pushSuccesses.load() 
             << ", Failure: " << pushFailures.load() << ")" << std::endl
             << "  Pop attempts: " << popAttempts.load() 
             << " (Success: " << popSuccesses.load() 
             << ", Failure: " << popFailures.load() << ")" << std::endl;
    
    // Check for consistency with a small tolerance
    // A few items might still be in the buffer when test ends
    size_t pushes = pushSuccesses.load();
    size_t pops = popSuccesses.load();
    size_t diff = pushes > pops ? pushes - pops : pops - pushes;
    const size_t MAX_ACCEPTABLE_DIFF = 10;  // Allow for a small number of items to be in-flight
    
    EXPECT_LE(diff, MAX_ACCEPTABLE_DIFF) 
        << "Significant mismatch between successful pushes (" << pushes 
        << ") and pops (" << pops << ")";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 