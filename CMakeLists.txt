cmake_minimum_required(VERSION 3.5)

project(BNAManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost QUIET REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        filetypes/manageable.h
        filetypes/bna.h
        filetypes/bna.cpp
        filetypes/msg.h
        filetypes/msg.cpp
        filetypes/scb.h
        filetypes/scb.cpp
        utility/stdhacks.h
        utility/stdhacks.cpp
        utility/datatools.h
        utility/streamtools.h
        utility/logger.h
        utility/logger.cpp
        about.h
        about.cpp
        about.ui
        filetablemodel.h
        filetablemodel.cpp
        filetableview.h
        filetableview.cpp
        bnacommontypes.h
)

set(APP_VERSION 0.5)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BNAManager
        WIN32
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET BNAManager APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(BNAManager SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(BNAManager
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(BNAManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(BNAManager PROPERTIES
    OUTPUT_NAME BNATool
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BNAManager)
endif()

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    #add_executable(progname file1.cxx file2.cxx)
    target_link_libraries(BNAManager ${Boost_LIBRARIES})
endif()
