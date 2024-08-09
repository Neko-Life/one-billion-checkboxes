cmake_minimum_required(VERSION 3.15)

project(atcboxes
	VERSION 0.0.1
	LANGUAGES CXX
	DESCRIPTION "A Trillion Checkboxes - Web App")

option(DEBUG_SYMBOL "Build ${PROJECT_NAME} with debug symbol" ON)
option(ACTUALLY_A_TRILLION "Build ${PROJECT_NAME} with actually a TRILLION checkbox state (requiring 125GB of memory)" OFF)

set(PROJ_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

file(GLOB_RECURSE PROJ_HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/*.h)

file(GLOB_RECURSE PROJ_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/*.cpp)

# Create an executable
add_executable(${PROJECT_NAME}
	${PROJ_HEADER_FILES}
	libs/uWebSockets/uSockets/src/libusockets.h
	libs/uWebSockets/uSockets/src/quic.h

	${PROJ_SOURCE_FILES}
	src/main.cpp)

set(USOCKETS_OBJECT_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/bsd.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/context.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/epoll_kqueue.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/gcd.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/io_context.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/io_loop.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/io_socket.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/libuv.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/loop.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/openssl.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/quic.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/socket.o
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/udp.o
	)

file(GLOB_RECURSE USOCKETS_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets/src/*.c)

file(GLOB_RECURSE UWEBSOCKETS_HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/src/*.h)

add_custom_target(uWebSockets
	DEPENDS ${USOCKETS_OBJECT_FILES})

add_custom_command(OUTPUT ${USOCKETS_OBJECT_FILES}
	COMMAND WITH_OPENSSL=0 WITH_ZLIB=1 make
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/uWebSockets/uSockets
	DEPENDS ${USOCKETS_SOURCE_FILES} ${UWEBSOCKETS_HEADER_FILES})

add_dependencies(${PROJECT_NAME} uWebSockets)

if (DEBUG_SYMBOL)
	message("-- INFO: Will build ${PROJECT_NAME} with debug symbol")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

if (ACTUALLY_A_TRILLION)
	message("-- INFO: Will build ${PROJECT_NAME} with a TRILLION checkbox state, this will require 125GB of memory")
	target_compile_definitions(${PROJECT_NAME} PUBLIC ACTUALLY_A_TRILLION)
else()
	message("-- INFO: Will build ${PROJECT_NAME} with a BILLION checkbox state")
endif()


target_link_libraries(${PROJECT_NAME}
	${USOCKETS_OBJECT_FILES}
	z
	pthread
	)

target_include_directories(${PROJECT_NAME} PRIVATE
	include
	libs
	libs/uWebSockets/uSockets/src
	# libs/jwt-cpp/include
	)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD ${PROJ_CXX_STANDARD}
	CXX_STANDARD_REQUIRED ON
	EXPORT_COMPILE_COMMANDS ON
	)

# vim: sw=8 noet
