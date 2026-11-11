cmake_minimum_required(VERSION 3.15)
project(effectivemoderncpp)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_FLAGS -fno-elide-constructors)

add_subdirectory(1.DeducingTypes)
add_subdirectory(2.Auto)
add_subdirectory(3.MovingToModernCpp)
add_subdirectory(4.SmartPointers)
add_subdirectory(5.RRefMovSemPerfForw)
add_subdirectory(6.LambdaExpressions)
add_subdirectory(8.Tweaks)
 