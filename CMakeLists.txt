cmake_minimum_required(VERSION 3.10)

project(webdav VERSION 1.0.0)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    ADD_DEFINITIONS(-DZLIB_WINAPI)
endif()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/SocketLib)
include_directories(${CMAKE_SOURCE_DIR}/tinyxml2)

add_subdirectory(FastCgi)
add_subdirectory(SocketLib)
add_subdirectory(tinyxml2)

if(UA_ENABLE_ENCRYPTION_OPENSSL)
    link_directories(${OPENSSL_LIB_DIR})
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options(/EHa)
endif()

set(targetSrc
        ${CMAKE_CURRENT_LIST_DIR}/webdav.cpp
        ${CMAKE_CURRENT_LIST_DIR}/md5/md5.cpp
)
add_executable(${PROJECT_NAME} ${targetSrc})
target_link_libraries(${PROJECT_NAME} socketlib)
target_link_libraries(${PROJECT_NAME} FastCgi)
target_link_libraries(${PROJECT_NAME} tinyxml2)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
if(UA_ENABLE_ENCRYPTION_OPENSSL)
    target_link_libraries(${PROJECT_NAME} libcrypto)
    target_link_libraries(${PROJECT_NAME} libssl)
endif()
else()
if(UA_ENABLE_ENCRYPTION_OPENSSL)
    target_link_libraries(${PROJECT_NAME} crypto)
    target_link_libraries(${PROJECT_NAME} ssl)
endif()
    target_link_libraries(${PROJECT_NAME} pthread)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
