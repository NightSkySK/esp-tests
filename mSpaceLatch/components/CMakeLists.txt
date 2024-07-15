cmake_minimum_required(VERSION 3.16)

idf_component_register(
        SRCS "buzzer.c"
        INCLUDE_DIRS "include"
        REQUIRES log freertos driver
)