cmake_minimum_required(VERSION 3.16)

project(chess VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appchess
    main.cpp
    mainwindow.cpp
    mainwindow.h
    chessboardwidget.cpp
    chessboardwidget.h
    tourmapwindow.cpp
    tourmapwindow.h
    horse.h
    horse.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appchess PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appchess
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appchess
    PRIVATE Qt6::Core Qt6::Widgets
)

include(GNUInstallDirs)
install(TARGETS appchess
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
