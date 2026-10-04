FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(jane60_tests
    unit/CalibrationTests.cpp)

target_link_libraries(jane60_tests PRIVATE jane60_dsp Catch2::Catch2WithMain)
target_compile_definitions(jane60_tests PRIVATE
    JANE60_CALIBRATION_FILE="${PROJECT_SOURCE_DIR}/calibration/juno60.json")

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(jane60_tests)
