include_directories(${CMAKE_SOURCE_DIR}/src)

hip_add_executable(hpcg_test rochpcg_gtest_main.cpp test_rochpcg.cpp)
target_link_libraries(hpcg_test PRIVATE rochpcgl GTest::GTest)
set_property(TARGET hpcg_test PROPERTY HIP_ARCHITECTURES ${AMDGPU_TARGETS})

add_executable(geomtest geometry.cpp)
target_link_libraries(geomtest PRIVATE rochpcgl GTest::gtest_main)

gtest_discover_tests(geomtest)
