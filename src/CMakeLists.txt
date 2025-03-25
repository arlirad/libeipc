add_library(eipc STATIC eipc.cpp endpoint.cpp)
target_compile_options(eipc PRIVATE -fcoroutines)