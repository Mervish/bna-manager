cmake_minimum_required(VERSION 3.5)

project(BNAManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED json)

add_subdirectory(thirdparty/OpenXLSX/OpenXLSX)

set(SCB_FILES
    filetypes/scb.cpp
    filetypes/scb.h
    filetypes/msg.h
    filetypes/msg.cpp
    filetypes/lbl.h
    filetypes/lbl.cpp
)

set(BXR_files
    filetypes/bxr.cpp
    filetypes/bxr.h
)

set(NUT_FILES
    filetypes/nut.h
    filetypes/nut.cpp
)

set(NFH_FILES
    filetypes/nfh.h
    filetypes/nfh.cpp
)

set(APP_VERSION 0.6)

qt_add_executable(BNAGUI
    WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    filetypes/manageable.h
    filetypes/bna.h
    filetypes/bna.cpp
    ${SCB_FILES}
    ${BXR_files}
    ${NUT_FILES}
    ${NFH_FILES}
    filetypes/nfh.h
    filetypes/nfh.cpp
    utility/datatools.h
    utility/streamtools.h
    utility/logger.h
    utility/logger.cpp
    utility/path.h
    utility/pathsaver.h
    utility/pathsaver.cpp
    utility/result.h
    about.h
    about.cpp
    about.ui
    filetablemodel.h
    filetablemodel.cpp
    filetableview.h
    filetableview.cpp
    utility/stringtools.h
)

target_include_directories(BNAGUI PRIVATE ./../libsquish)

set_target_properties(BNAGUI PROPERTIES
    OUTPUT_NAME BNAGUI
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BNAGUI)
endif()

add_executable(bnamaster
    tools/master.cpp
    filetypes/bna.h
    filetypes/bna.cpp
    ${SCB_FILES}
    ${NUT_FILES}
    filetypes/scenario.h
    filetypes/scenario.cpp
)

add_executable(bxrtool
    tools/bxrconvert.cpp
    ${BXR_files}
)

add_executable(bnacmdtool
    tools/bnaunpack.cpp
    filetypes/bna.h
    filetypes/bna.cpp
)

add_executable(nuttool
    tools/nutunpack.cpp
    ${NUT_FILES}
    utility/commandline.h
)

add_executable(imaspatcher
    tools/imaspatcher.cpp
    filetypes/bna.h
    filetypes/bna.cpp
    ${BXR_files}
    ${SCB_FILES}
    ${NUT_FILES}
    filetypes/scenario.h
    filetypes/scenario.cpp
    utility/commandline.h
    utility/filetype.h
)

add_executable(scbtool
    tools/scbextract.cpp
    ${SCB_FILES}
)

add_executable(nfhtool
    tools/nfhtool.cpp
    ${NFH_FILES}
)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(BNAGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        ${Boost_LIBRARIES}
        OpenXLSX::OpenXLSX)
    target_link_libraries(imaspatcher PRIVATE ${Boost_LIBRARIES} OpenXLSX::OpenXLSX)
    target_link_libraries(bnamaster ${Boost_LIBRARIES} OpenXLSX::OpenXLSX)
    target_link_libraries(nuttool ${Boost_LIBRARIES})
    target_link_libraries(scbtool ${Boost_LIBRARIES} OpenXLSX::OpenXLSX)
    target_link_libraries(nfhtool ${Boost_LIBRARIES})
endif()


