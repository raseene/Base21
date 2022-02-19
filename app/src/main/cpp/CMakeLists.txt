# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.18.1)
project("native")

add_subdirectory(sys)

add_library(
	native
	SHARED

	android_main.cpp
	SceneTest.cpp
)

target_include_directories(
	native

	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
	native

	system-lib
)
