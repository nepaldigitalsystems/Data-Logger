idf_component_register(SRCS "client.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                    driver 
                    esp_http_client
                    json)
