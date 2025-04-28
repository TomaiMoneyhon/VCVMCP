cmake_minimum_required(VERSION 3.15)

# Create a convenience function to reduce repetition and ensure consistent linking
function(add_mcp_test_executable target_name source_files)
  add_executable(${target_name} ${source_files})
  target_link_libraries(${target_name} PRIVATE mcp gtest_main)
  add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

# Main broker tests
add_mcp_test_executable(broker_tests 
  mcp/BrokerTests.cpp
)

# Serialization tests
add_mcp_test_executable(serialization_tests
  mcp/SerializationTests.cpp
)

# Publish/Subscribe tests  
add_mcp_test_executable(pubsub_tests
  mcp/PublishSubscribeTests.cpp
)

# Ring buffer tests
add_mcp_test_executable(ringbuffer_tests
  mcp/RingBufferTests.cpp
)

# Reference implementation tests
add_mcp_test_executable(reference_tests
  mcp/ReferenceImplementationTests.cpp
) 