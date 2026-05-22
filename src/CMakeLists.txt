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
    fft/fft_scalar.cpp
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

find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIAUDIO REQUIRED miniaudio)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(PULSE REQUIRED libpulse)
pkg_check_modules(JACK REQUIRED jack)

target_include_directories(spectra_eq PRIVATE
        ${MINIAUDIO_INCLUDE_DIRS}
        ${ALSA_INCLUDE_DIRS}
        ${PULSE_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
)
target_link_libraries(spectra_eq PRIVATE
        ${CMAKE_PROJECT_NAME}::core
        ${MINIAUDIO_LINK_LIBRARIES}
        ${ALSA_LINK_LIBRARIES}
        ${PULSE_LINK_LIBRARIES}
        ${JACK_LINK_LIBRARIES}
)
target_compile_options(spectra_eq PRIVATE ${MINIAUDIO_CFLAGS_OTHER})