idf_component_register(SRCS "sleep-test.c"
                    INCLUDE_DIRS "."
                    REQUIRES lwip
                             nvs_flash)                           