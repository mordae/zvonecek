cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(zvonecek)

fatfs_create_partition_image(storage fat FLASH_IN_PROJECT)
