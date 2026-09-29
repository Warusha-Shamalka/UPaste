cmake_minimum_required(VERSION 3.16)
project(ClipboardManager LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

add_executable(clipboard_manager
    src/main.c
    src/app.c
    src/database.c
    src/clipboard_monitor.c
    src/window.c
)

target_include_directories(clipboard_manager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(clipboard_manager PRIVATE
    PkgConfig::GTK
    PkgConfig::SQLITE3
)

install(TARGETS clipboard_manager
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES packaging/upaste.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

add_custom_target(run
    COMMAND $<TARGET_FILE:clipboard_manager>
    DEPENDS clipboard_manager
    COMMENT "Running clipboard manager"
)