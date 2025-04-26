add_executable(broker_tests
  mcp/BrokerTests.cpp
)

target_link_libraries(broker_tests
  PRIVATE
  mcp
  gtest
  gtest_main
)

add_test(NAME broker_tests COMMAND broker_tests)

# Add serialization tests
add_executable(serialization_tests
  mcp/SerializationTests.cpp
)

target_link_libraries(serialization_tests
  PRIVATE
  mcp
  gtest
  gtest_main
)

add_test(NAME serialization_tests COMMAND serialization_tests) 