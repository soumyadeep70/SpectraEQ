find_package(GTest REQUIRED)
include(GoogleTest)

set(TEST_SOURCES
    test_fft_scalar.cpp
)

foreach(source_file ${TEST_SOURCES})
    get_filename_component(test_name ${source_file} NAME_WE)

    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE ${CMAKE_PROJECT_NAME}::core GTest::gtest_main)

    gtest_discover_tests(${test_name})
    set_target_properties(${test_name} PROPERTIES FOLDER "tests")
endforeach()