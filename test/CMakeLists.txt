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