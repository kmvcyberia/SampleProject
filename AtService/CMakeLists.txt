project(AtService LANGUAGES CXX)

add_executable(AtService)

target_link_libraries(AtService
        PRIVATE
        Sample::Core
        Sample::Error
        Sample::Tty
)

target_sources(AtService
        PRIVATE
        src/at_service/core/app/app.cpp
        src/at_service/core/app/app_config.cpp
        src/main.cpp
        PRIVATE
        FILE_SET source_modules TYPE CXX_MODULES BASE_DIRS src FILES
        src/at_service/core/app/app.cppm
        src/at_service/core/app/app_config.cppm
        src/at_service/core/utility/file/file.cppm
)

set(CONFIG_DEST "${CMAKE_CURRENT_BINARY_DIR}/config.json")
set(CONFIG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/config_example.json")
if (NOT EXISTS "${CONFIG_DEST}")
    configure_file("${CONFIG_SRC}" "${CONFIG_DEST}" COPYONLY)
endif ()
set(DICTIONARY_DEST "${CMAKE_CURRENT_BINARY_DIR}/dictionary.csv")
set(DICTIONARY_SRC "${CMAKE_CURRENT_SOURCE_DIR}/dictionary_example.csv")
if (NOT EXISTS "${DICTIONARY_DEST}")
    configure_file("${DICTIONARY_SRC}" "${DICTIONARY_DEST}" COPYONLY)
endif ()
