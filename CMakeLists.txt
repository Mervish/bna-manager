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

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED json)

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
        filetypes/bxr.h
        filetypes/bxr.cpp
        filetypes/nut.h
        filetypes/nut.cpp
        utility/stdhacks.h
        utility/stdhacks.cpp
        utility/datatools.h
        utility/streamtools.h
        utility/logger.h
        utility/logger.cpp
        utility/path.h
        utility/pathsaver.h
        utility/pathsaver.cpp
        about.h
        about.cpp
        about.ui
        filetablemodel.h
        filetablemodel.cpp
        filetableview.h
        filetableview.cpp
)

set(APP_VERSION 0.5)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BNATool
        WIN32
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(BNATool SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(BNATool
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(BNATool PRIVATE ./../libsquish)

set_target_properties(BNATool PROPERTIES
    OUTPUT_NAME BNATool
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BNATool)
endif()

qt_add_executable(BNAMaster
    WIN32
    tools/master.cpp
    filetypes/bna.h
    filetypes/bna.cpp
    filetypes/scb.h
    filetypes/scb.cpp
    filetypes/msg.h
    filetypes/msg.cpp
    filetypes/scenario.h
    filetypes/scenario.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BNAMaster)
endif()

add_executable(bxrconvert
    WIN32
    tools/bxrconvert.cpp
    filetypes/bxr.h
    filetypes/bxr.cpp
)

target_link_libraries(bxrconvert PRIVATE Qt${QT_VERSION_MAJOR}::Core)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(bxrconvert)
endif()

add_executable(bnaunpack
    WIN32
    tools/bnaunpack.cpp
    filetypes/bna.h
    filetypes/bna.cpp
)

add_executable(nutunpack
    tools/nutunpack.cpp
    filetypes/nut.h
    filetypes/nut.cpp
    utility/commandline.h)

add_executable(imaspatcher
    tools/imaspatcher.cpp
    filetypes/bna.h
    filetypes/bna.cpp
    filetypes/bxr.h
    filetypes/bxr.cpp
    filetypes/scb.h
    filetypes/scb.cpp
    filetypes/msg.h
    filetypes/msg.cpp
    filetypes/nut.h
    filetypes/nut.cpp
    filetypes/scenario.h
    filetypes/scenario.cpp
    utility/commandline.h
    utility/filetype.h)

add_executable(scbextract
    tools/scbextract.cpp
    filetypes/scb.cpp
    filetypes/scb.h
    filetypes/msg.cpp
    filetypes/msg.h)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(BNATool PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        ${Boost_LIBRARIES})
    target_link_libraries(imaspatcher PRIVATE Qt${QT_VERSION_MAJOR}::Core ${Boost_LIBRARIES})
    target_link_libraries(BNAMaster PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${Boost_LIBRARIES})
    target_link_libraries(nutunpack ${Boost_LIBRARIES})
    target_link_libraries(scbextract ${Boost_LIBRARIES})
endif()


