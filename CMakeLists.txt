cmake_minimum_required(VERSION 3.10)

project(webdav VERSION 1.0.0)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    ADD_DEFINITIONS(-DZLIB_WINAPI)
    # Turn off Microsofts "security" warnings.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CONSOLE _SCL_SECURE_NO_WARNINGS)
else()
    # add_compile_options("$<$<CONFIG:Debug>: -O0>")
    SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -ggdb")
    add_compile_options("-Wall" "-Wpedantic" "-Wextra" "-fexceptions")
endif()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/SocketLib)
include_directories(${CMAKE_SOURCE_DIR}/tinyxml2)

file(READ "webdav.cpp" SrcFile)
string(REGEX MATCH "szSrvName = L\"([^\"]*)\"" _ ${SrcFile})
set(SrvName ${CMAKE_MATCH_1})
message(STATUS "ServiceName: ${SrvName}")

add_subdirectory(FastCgi EXCLUDE_FROM_ALL)
add_subdirectory(SocketLib EXCLUDE_FROM_ALL)
add_subdirectory(tinyxml2 EXCLUDE_FROM_ALL)
add_subdirectory(SrvLib EXCLUDE_FROM_ALL)

if(UA_ENABLE_ENCRYPTION_OPENSSL)
    link_directories(${OPENSSL_LIB_DIR})
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options(/EHa)
endif()

set(targetSrc
        ${CMAKE_CURRENT_LIST_DIR}/webdav.cpp
        ${CMAKE_CURRENT_LIST_DIR}/md5/md5.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ConfFile.cpp
)
add_executable(${PROJECT_NAME} ${targetSrc})

target_link_libraries(${PROJECT_NAME} socketlib)
target_link_libraries(${PROJECT_NAME} FastCgi)
target_link_libraries(${PROJECT_NAME} tinyxml2)
target_link_libraries(${PROJECT_NAME} srvlib)

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

if (NOT MSVC)
    if (NOT EXISTS(${CMAKE_CURRENT_BINARY_DIR}/${SrvName}srv))
        file(READ SrvLib/init.d/examplesrv FILE_CONTENTS)
        string(REPLACE "ExampleSrv" ${SrvName} FILE_CONTENTS ${FILE_CONTENTS})
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${SrvName}srv ${FILE_CONTENTS})
    endif()

    if (NOT EXISTS(${CMAKE_CURRENT_BINARY_DIR}/${SrvName}.service))
        file(READ SrvLib/example.service FILE_CONTENTS)
        string(REPLACE "ExampleSrv" ${SrvName} FILE_CONTENTS ${FILE_CONTENTS})
        string(REPLACE "~" ${CMAKE_SOURCE_DIR} FILE_CONTENTS ${FILE_CONTENTS})
        string(REPLACE "Some Example server" "webdav Service" FILE_CONTENTS ${FILE_CONTENTS})
        string(REPLACE "example/" "" FILE_CONTENTS ${FILE_CONTENTS})
        string(REPLACE "example" "" FILE_CONTENTS ${FILE_CONTENTS})
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${SrvName}.service ${FILE_CONTENTS})
    endif()
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
