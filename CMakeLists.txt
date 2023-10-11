cmake_minimum_required(VERSION 2.8)
project(ConnPool)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)


set(
    LIB_LIB
    pthread
    hiredis
    mysqlcppconn
)


add_executable(redishelper_test tests/redishelper_test.cpp)
target_link_libraries(redishelper_test ${LIB_LIB})

add_executable(redispool_test tests/redispool_test.cpp)
target_link_libraries(redispool_test ${LIB_LIB})

add_executable(mysqltest tests/mysqltest.cpp)
target_link_libraries(mysqltest ${LIB_LIB})

add_executable(table_test tests/table_test.cpp)
target_link_libraries(table_test ${LIB_LIB})



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)