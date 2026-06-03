add_library(spectra_eq_compile_options INTERFACE)
target_compile_options(spectra_eq_compile_options INTERFACE
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wpedantic
        -Wno-unused-parameter
        -fno-omit-frame-pointer

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
        audio/audio_processor.cpp
)
add_library(spectra_eq_core STATIC ${CORE_LIB_SOURCES})
add_library(${CMAKE_PROJECT_NAME}::core ALIAS spectra_eq_core)
target_include_directories(spectra_eq_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_link_libraries(spectra_eq_core PUBLIC spectra_eq_compile_options)
target_compile_features(spectra_eq_core PUBLIC cxx_std_23)

find_package(Qt6 REQUIRED COMPONENTS
        Quick
        QuickControls2
)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(spectra_eq main.cpp)

qt_add_qml_module(spectra_eq
        URI SpectraEQ
        VERSION 1.0
        QML_FILES
            ui/Main.qml
        SOURCES
            ${CMAKE_SOURCE_DIR}/include/ui/AppState.h
)

qt_add_resources(spectra_eq "assets"
        FILES
        ui/assets/app_icon.png
        PREFIX "/"
)

target_link_libraries(spectra_eq PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)
target_link_libraries(spectra_eq PRIVATE ${CMAKE_PROJECT_NAME}::core)
target_include_directories(spectra_eq PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/ui>) # hack for qt

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
        ${MINIAUDIO_LINK_LIBRARIES}
        ${ALSA_LINK_LIBRARIES}
        ${PULSE_LINK_LIBRARIES}
        ${JACK_LINK_LIBRARIES}
)
target_compile_options(spectra_eq PRIVATE ${MINIAUDIO_CFLAGS_OTHER})