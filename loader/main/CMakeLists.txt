idf_component_register(SRCS "main.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES settings-manager wifi-connect-lite esp_http_server app_update
                        EMBED_FILES "index.html")