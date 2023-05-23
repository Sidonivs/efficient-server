project(server)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} "server.cpp" "streamwrapper.cpp" "threadwrapper.cpp" "acceptor.cpp" "gen/schema.pb.cc" "graph.cpp" ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(${PROJECT_NAME} ${Protobuf_LIBRARIES})