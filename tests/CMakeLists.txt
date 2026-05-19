find_package(Catch2 3 REQUIRED)
include(Catch)

set(TEST_SOURCES
    main.cpp
)

foreach(source_file ${TEST_SOURCES})
    get_filename_component(test_name ${source_file} NAME_WE)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE ${CMAKE_PROJECT_NAME}::core)
    target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(${test_name})
    set_target_properties(${test_name} PROPERTIES FOLDER "tests")
endforeach()