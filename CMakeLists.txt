cmake_minimum_required(VERSION 3.17)
project(DatasetMaker)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OpenCV_DIR "/home/lovod/opencv-4.8.1/build")

# 启用 Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(OpenCV 4.8.1 REQUIRED)

# 包含构建目录
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OPENCV_INCLUDE_DIRS})

# 查找 Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# 设置文件路径 - 根据实际目录调整
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(UI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ui)

file(GLOB SOURCES ${SOURCE_DIR}/*.cpp)
file(GLOB HEADERS ${HEADER_DIR}/*.h)
file(GLOB FORMS ${UI_DIR}/*.ui)

# 显式生成UI头文件
qt5_wrap_ui(UI_HEADERS
        ${UI_DIR}/cls.ui
        ${UI_DIR}/datasetmaker.ui
        ${UI_DIR}/detection.ui
        ${UI_DIR}/leftPart.ui
)

# 添加可执行文件
add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        ${UI_HEADERS}
        include/common.h
        src/common.cpp
        src/autoMode.cpp
        include/autoMode.h
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        ${OpenCV_LIBS}
)

# 设置生成UI头文件的输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
        AUTOUIC_SEARCH_PATHS ${UI_DIR}
        AUTOUIC_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/autogen/ui
)

# 包含生成的UI头文件目录
target_include_directories(
        ${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/autogen/ui
)

# 添加翻译文件
find_package(Qt5LinguistTools)
qt5_add_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/DataserMaker_zh_CN.ts)
add_custom_target(translations ALL DEPENDS ${QM_FILES})
