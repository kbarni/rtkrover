cmake_minimum_required(VERSION 3.19)
project(rtkrover LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core SerialPort Network)

qt_standard_project_setup()

qt_add_executable(rtkrover
    main.cpp
    crtkrover.h crtkrover.cpp
    casterreader.h casterreader.cpp
    serialcom.h serialcom.cpp
    gpsdataparser.h gpsdataparser.cpp
    crc24q.h
    Todo.md
    README.md
    Changelog.md
)

target_link_libraries(rtkrover PRIVATE Qt6::Core Qt6::SerialPort Qt6::Network)

include(GNUInstallDirs)

install(TARGETS rtkrover
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
