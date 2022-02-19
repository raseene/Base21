# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.18.1)
project("system-lib")

find_package(game-activity REQUIRED CONFIG)
find_package(games-frame-pacing REQUIRED CONFIG)
#find_package(games-performance-tuner REQUIRED CONFIG)

add_library(
	system-lib
	STATIC

	Engine.cpp
	Scene.cpp
	Renderer.cpp
	Shader.cpp
	Texture.cpp
	FrameBuffer.cpp
	Sprite.cpp
	TouchPanel.cpp
	Sound.cpp
	included.cpp
	included.c
	libpng/jni/png.c
	libpng/jni/pngerror.c
	libpng/jni/pngget.c
	libpng/jni/pngmem.c
	libpng/jni/pngpread.c
	libpng/jni/pngread.c
	libpng/jni/pngrio.c
	libpng/jni/pngrtran.c
	libpng/jni/pngrutil.c
	libpng/jni/pngset.c
	libpng/jni/pngtrans.c
	libpng/jni/pngwio.c
	libpng/jni/pngwrite.c
	libpng/jni/pngwtran.c
	libpng/jni/pngwutil.c
	libpng/jni/arm/arm_init.c
	libpng/jni/arm/filter_neon.S
	libpng/jni/arm/filter_neon_intrinsics.c
	Tremor/block.c
	Tremor/codebook.c
	Tremor/floor0.c
	Tremor/floor1.c
	Tremor/info.c
	Tremor/mapping0.c
	Tremor/mdct.c
	Tremor/registry.c
	Tremor/res012.c
	Tremor/sharedbook.c
	Tremor/synthesis.c
	Tremor/vorbisfile.c
	Tremor/window.c
	Tremor/src/bitwise.c
	Tremor/src/framing.c
)

target_include_directories(
	system-lib

	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Tremor
)

find_library(
	log-lib

	log
)

target_link_libraries(
	system-lib

	android
	z
	EGL
	GLESv2
	OpenSLES
	game-activity::game-activity
	games-frame-pacing::swappy_static
#	games-performance-tuner::tuningfork_static
	${log-lib}
)