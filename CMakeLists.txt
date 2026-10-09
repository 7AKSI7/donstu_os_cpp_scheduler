cmake_minimum_required(VERSION 3.16)

project(donstu_os_cpp_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(donstu_os_cpp_scheduler main.cpp
    process.h
    scheduler.h
    fcfs.h
    sjf.h
    srtn.h
    rr.h
    priority.h
    mlfq.h
    simulator.h
    simulator.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(donstu_os_cpp_scheduler PRIVATE -Wall -Wextra)
endif()

include(GNUInstallDirs)
install(TARGETS donstu_os_cpp_scheduler
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
