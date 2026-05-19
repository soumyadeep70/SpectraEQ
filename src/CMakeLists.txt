add_library(spectra_eq_compile_options INTERFACE)
target_compile_options(spectra_eq_compile_options INTERFACE
    -Wall
    -Wextra
    -Wshadow
    -Wconversion
    -Wpedantic
    -Wno-unused-parameter
    -fno-omit-frame-pointer

    # Build-type-specific flags
    $<$<CONFIG:Debug>:
    -Og
    -g
    -fsanitize=address,undefined
    >
    $<$<CONFIG:Release>:
    -O3
    -march=native
    -funroll-loops
    -DNDEBUG
    >
)
target_link_options(spectra_eq_compile_options INTERFACE
    $<$<CONFIG:Debug>:
    -fsanitize=address,undefined
    >
)


set(CORE_LIB_SOURCES
    fft/fft.cpp
)
add_library(spectra_eq_core STATIC ${CORE_LIB_SOURCES})
add_library(${CMAKE_PROJECT_NAME}::core ALIAS spectra_eq_core)
target_include_directories(spectra_eq_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
target_link_libraries(spectra_eq_core PUBLIC spectra_eq_compile_options)
target_compile_features(spectra_eq_core PUBLIC cxx_std_23)


set(EXE_SOURCES
    main.cpp
)
add_executable(spectra_eq ${EXE_SOURCES})
target_link_libraries(spectra_eq PRIVATE ${CMAKE_PROJECT_NAME}::core)