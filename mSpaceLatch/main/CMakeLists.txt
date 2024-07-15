if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    list(APPEND PRIVREQ esp_adc)
else()
    list(APPEND PRIVREQ esp_adc_cal)
endif()


idf_component_register(SRCS "blink_codes.c" "mSpaceLatch.c" "motor_lock.c" "mspacelock.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ${PRIVREQ}
                             driver
                             components
                             esp_timer)
