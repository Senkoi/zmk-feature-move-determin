zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MOVE_DETERMIN src/move_determin.c)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)