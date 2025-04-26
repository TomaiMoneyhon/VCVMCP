add_executable(broker_registration
  broker-registration/main.cpp
)

target_link_libraries(broker_registration
  PRIVATE
  mcp
) 