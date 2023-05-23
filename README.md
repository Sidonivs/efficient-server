# ESW Efficient Server
## Requirements
- CMake (I use version 3.16.3)
- Protobuf compiler and C++ library (v3.20.0 obtained from [https://github.com/protocolbuffers/protobuf/releases/latest
](https://github.com/protocolbuffers/protobuf/releases/latest
))

## Installation
### Server (C++)
To generate the required protobuf C++ code do this first (regenerate if building on a new machine):
```
mkdir gen
protoc --proto_path=proto --cpp_out=gen proto/schema.proto
```
Then:
```
mkdir build
cd build
cmake ..
make
```
This will build the application with cmake.

## Run
To run the server:
```
cd build
./server 100 9999
```
Arguments are: 
1. number of threads 
2. port
3. ip address (not required if run on localhost)