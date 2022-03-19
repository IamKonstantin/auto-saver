cmake_minimum_required(VERSION 3.5)

project(auto-saver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)


set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        applybutton.cpp
        applybutton.h
)
add_executable(auto-saver WIN32 ${PROJECT_SOURCES} )
target_link_libraries(auto-saver PRIVATE Qt${QT_VERSION_MAJOR}::Widgets )


install(
    TARGETS
        auto-saver
    DESTINATION
        ".")
install(
    FILES
        ${CMAKE_PREFIX_PATH}/bin/Qt5Core.dll
        ${CMAKE_PREFIX_PATH}/bin/Qt5Gui.dll
        ${CMAKE_PREFIX_PATH}/bin/Qt5Widgets.dll
    DESTINATION
        "."
)
install(
    FILES
        ${CMAKE_PREFIX_PATH}/plugins/platforms/qwindows.dll
    DESTINATION
        plugins/platforms
)
install(
    FILES
        ${CMAKE_PREFIX_PATH}/plugins/styles/qwindowsvistastyle.dll
    DESTINATION
        plugins/styles
)
set(WINDOWS_SYTSTEM32 C:/Windows/System32)
install(
    FILES
        ${WINDOWS_SYTSTEM32}/vcruntime140.dll
        ${WINDOWS_SYTSTEM32}/vcruntime140_1.dll
        ${WINDOWS_SYTSTEM32}/vcruntime140_clr0400.dll
    DESTINATION
        "."
)
