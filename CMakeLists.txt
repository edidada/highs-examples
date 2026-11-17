cmake_minimum_required(VERSION 3.16)
project(highs_data_interface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The HiGHS SDK is provided through CMAKE_PREFIX_PATH in CI.
find_package(highs CONFIG REQUIRED)

add_library(hdi STATIC src/highs_data_interface.cpp)
target_include_directories(hdi PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(hdi PUBLIC highs::highs)

add_executable(highs_cli src/main.cpp)
target_link_libraries(highs_cli PRIVATE hdi)

include(CTest)
if(BUILD_TESTING)
  add_executable(hdi_test tests/test_solve.cpp)
  target_link_libraries(hdi_test PRIVATE hdi)
  add_test(NAME hdi_test
           COMMAND hdi_test "${CMAKE_CURRENT_SOURCE_DIR}/data/example.lpdat")
endif()
