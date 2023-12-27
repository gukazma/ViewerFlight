# 查找 Qt 包信息备用
set(TARGET_NAME ViewerFlight)


find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL CONFIG REQUIRED)

# 开启 MOC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(osgearth)
find_package(LibKML REQUIRED)
find_package(Boost REQUIRED)

# 扫描当前源代码文件列表

file(GLOB_RECURSE CPP_HEAD_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE CPP_SOUCE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE UI_SOUCE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
set(OSGQT_HEAD 
${CMAKE_CURRENT_SOURCE_DIR}/Vendors/osgQt/osgQOpenGL/osgQOpenGLWidget
${CMAKE_CURRENT_SOURCE_DIR}/Vendors/osgQt/osgQOpenGL/osgQOpenGLWindow
${CMAKE_CURRENT_SOURCE_DIR}/Vendors/osgQt/osgQOpenGL/OSGRenderer
)
QT5_WRAP_CPP(MOC_SOURCES ${OSGQT_HEAD})


source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CPP_HEAD_FILES} ${CPP_SOUCE_FILES} ${OSGQT_HEAD})

add_executable(${TARGET_NAME} ${CPP_HEAD_FILES} ${CPP_SOUCE_FILES} ${UI_SOUCE_FILES} ${MOC_SOURCES} ${OSGQT_HEAD})
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Apps)
# 链接 Conan 指向的 Qt 库文件
target_link_libraries(${TARGET_NAME} PRIVATE
Qt5::Core
Qt5::Gui
Qt5::Widgets
Qt5::OpenGL
Boost::system
Boost::filesystem
Boost::dynamic_linking
Boost::thread
CONAN_PKG::osgearth
kmlbase kmlconvenience kmldom kmlengine
uavmvs::uavmvs
)
target_include_directories(${TARGET_NAME} PRIVATE
${CMAKE_CURRENT_LIST_DIR}/Vendors/osgQt
${OPENSCENEGRAPH_INCLUDE_DIRS}
)


find_program(DEPLOYQT_EXECUTABLE windeployqt)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${DEPLOYQT_EXECUTABLE} ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME}.exe
  COMMENT "Deploying qt dependencies after build..."
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_LIST_DIR}/Resources
        ${CMAKE_BINARY_DIR}/bin
    COMMENT "Copying resources to output directory"
)