include_directories(${CMAKE_SOURCE_DIR}/src)

hip_add_executable(hpcg_test rochpcg_gtest_main.cpp test_rochpcg.cpp)
target_link_libraries(hpcg_test PRIVATE rochpcgl GTest::GTest)

add_executable(geomtest geometry.cpp)
target_link_libraries(geomtest PRIVATE rochpcgl GTest::gtest_main)

gtest_discover_tests(geomtest)
