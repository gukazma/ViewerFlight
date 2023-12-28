set(TARGET_NAME "uavmvs")

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE HPP_FILES "*.hpp")
file(GLOB_RECURSE CPP_FILES "*.cpp")
find_package(osgearth)
find_package(LibKML REQUIRED)
find_package(Boost REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL CONFIG REQUIRED)
find_package(TBB REQUIRED)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${HPP_FILES} ${CPP_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${CPP_FILES} ${HEADER_FILES} ${HPP_FILES})
add_library(uavmvs::uavmvs ALIAS ${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Modules)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

# Link dependencies    
target_link_libraries(${TARGET_NAME} 
    PUBLIC
Qt5::Core
Boost::system
Boost::filesystem
Boost::dynamic_linking
Boost::thread
CONAN_PKG::osgearth
TBB::TBB
kmlbase kmlconvenience kmldom kmlengine
          )

target_include_directories(
  ${TARGET_NAME} 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

set_target_properties(${TARGET_NAME}
    PROPERTIES
        FOLDER Modules/Common
)