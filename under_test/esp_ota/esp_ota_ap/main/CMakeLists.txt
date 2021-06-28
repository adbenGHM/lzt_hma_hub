set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../webpages")
idf_component_register(SRCS "app_httpServer.c"  "app_main.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "${WEB_SRC_DIR}/favicon.ico"
                    EMBED_FILES "${WEB_SRC_DIR}/setup/setup.html"
                )