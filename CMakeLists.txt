cmake_minimum_required(VERSION 3.16)
project(GrowCastleProgressTracker LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW documentation generation" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW test targets" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW example targets" FORCE)

# Fetch dependencies: GLFW and ImGui only
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
)
FetchContent_MakeAvailable(glfw imgui)

if (TARGET glfw AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(glfw PRIVATE -w)
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CORE_SOURCES
    ${SRC_DIR}/file_operations.c
    ${SRC_DIR}/player_stats.c
    ${SRC_DIR}/upgrading.c
    ${SRC_DIR}/investment.c
    ${SRC_DIR}/pace_analysis.c
    ${SRC_DIR}/gpw.c
)

set(GUI_SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/gui.cpp
)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include
)

target_link_libraries(imgui PRIVATE glfw)

add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${GUI_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${INC_DIR}
        ${glfw_SOURCE_DIR}/include
        ${imgui_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        imgui
        ${CMAKE_DL_LIBS}
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()

if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

enable_testing()

add_executable(pace_analysis_tests
    ${SRC_DIR}/pace_analysis.c
    tests/pace_analysis_tests.cpp
)

target_include_directories(pace_analysis_tests
    PRIVATE
        ${INC_DIR}
)

add_test(NAME pace_analysis_tests COMMAND pace_analysis_tests)

add_executable(gpw_tests
    ${SRC_DIR}/gpw.c
    tests/gpw_tests.cpp
)

target_include_directories(gpw_tests
    PRIVATE
        ${INC_DIR}
)

add_test(NAME gpw_tests COMMAND gpw_tests)

add_executable(investment_tests
    ${SRC_DIR}/upgrading.c
    ${SRC_DIR}/investment.c
    tests/investment_tests.cpp
)

target_include_directories(investment_tests
    PRIVATE
        ${INC_DIR}
)

add_test(NAME investment_tests COMMAND investment_tests)

add_executable(file_operations_tests
    ${SRC_DIR}/file_operations.c
    tests/file_operations_tests.cpp
)

target_include_directories(file_operations_tests
    PRIVATE
        ${INC_DIR}
)

set(FILE_OPERATIONS_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/file_operations_test_data)
file(MAKE_DIRECTORY ${FILE_OPERATIONS_TEST_DIR})
add_test(NAME file_operations_tests COMMAND file_operations_tests)
set_tests_properties(file_operations_tests PROPERTIES WORKING_DIRECTORY ${FILE_OPERATIONS_TEST_DIR})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
)
