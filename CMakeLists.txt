cmake_minimum_required(VERSION 3.22)

if(APPLE)
    # Fix runtime path for installed executables
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
endif()

project(PTVC_Project_GL)

include(FetchContent)

if(MSVC)
    # https://docs.microsoft.com/en-us/previous-versions/958x11bc(v=vs.140)
    add_compile_options("$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>")
    add_compile_options("$<$<CONFIG:Debug>:/ZI>$<$<CONFIG:Release>:/Zi>")
    add_compile_options(/FS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Qpar")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(UNIX AND NOT APPLE)
    set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux" ".")
elseif(UNIX AND APPLE)
    set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos" ".")
elseif(WIN32)
    set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows" ".")
endif()

find_package(OpenGL REQUIRED)

# Include Jolt
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
    GIT_TAG "v4.0.2"
    SOURCE_SUBDIR "Build")
FetchContent_MakeAvailable(JoltPhysics)

# Include ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)
FetchContent_MakeAvailable(imgui)
include_directories(${imgui_SOURCE_DIR}/)
set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/)
file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp)
file(GLOB IMGUI_HEADERS ${imgui_SOURCE_DIR}/*.h)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
# Maybe I can get this to work, but it's not really needed right now
# set(IMGUI_USER_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/imconfig.h")
# target_compile_definitions(imgui INTERFACE "-DIMGUI_USER_CONFIG=<${IMGUI_USER_CONFIG}>")

# Include LZ4
FetchContent_Declare(
    lz4
    GIT_REPOSITORY "https://github.com/lz4/lz4"
    GIT_TAG "v1.9.4"
)
FetchContent_MakeAvailable(lz4)
file(GLOB lz4_SOURCES ${lz4_SOURCE_DIR}/lib/*.c)
add_library(lz4 STATIC ${lz4_SOURCES})
include_directories(${lz4_SOURCE_DIR}/lib)

set(LINK_LIBRARIES
    debug glfw3d optimized glfw3
    debug glew32sd optimized glew32s
    OpenGL::GL
    Jolt
    imgui
    lz4
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${lz4_SOURCE_DIR}/lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBRARIES})

# IDE specific settings
if(CMAKE_GENERATOR MATCHES "Visual Studio")
   set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
   set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
   set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
elseif(CMAKE_GENERATOR MATCHES "Xcode")
   set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_GENERATE_SCHEME TRUE CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
   set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY XCODE_STARTUP_PROJECT ${PROJECT_NAME})
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
endif()

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

if(MSVC)
    # enable warnings except: 
    # 'size_t' possible loss of data, 'identifier' hides class member,  unused parameter
    # diable warnings in external includes
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /wd4267 /wd4458 /wd4100 /external:anglebrackets /external:W0)
endif()

install(
    TARGETS ${PROJECT_NAME} CONFIGURATIONS Debug DESTINATION "debug"
)
install(
    TARGETS ${PROJECT_NAME} CONFIGURATIONS Release DESTINATION "release"
)
