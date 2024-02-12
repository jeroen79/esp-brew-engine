idf_component_register(SRCS "brew-engine.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_http_server onewire_bus mqtt settings-manager app_update
                    EMBED_FILES "index.html.gz" "manifest.json" "logo.svg.gz")