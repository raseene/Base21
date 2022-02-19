libpng for Android for Castle Game Engine
==================

libpng code that can be easily recompiled for Android shared library, and copied to CGE sources.

Builds for all platforms relevant for _Castle Game Engine_:
- 32-bit ARM
- 64-bit ARM (aka Aarch64)
- 32-bit X86
- 64-bit X86 (x86_64)

Use `make build`. This calls `ndk-build` to compile the libraries and copies them to _Castle Game Engine_ tree (assumed in `$CASTLE_ENGINE_PATH`).

Consistent with other CGE Android native libs, like https://github.com/castle-engine/android-freetype and https://github.com/castle-engine/android-openal .

# Original README notes

Notes from https://github.com/julienr/libpng-android , that we base our fork upon:

This is a repackaging of libpng 1.6.29 for Android.

Most changes went in config.h and writing the Makefiles.

The original libpng website is : http://www.libpng.org/pub/png/libpng.html

Assuming 'ndk-build' is in your path, you can use the build.sh script to create a static library.

The 'master' branch of this repository contains upstream version 1.6.29. This
hasn't been tested much on Android.

The 'stable' branch of this repository contains the older 1.4.1 version which works fine on Android.

See the wiki_ for an example Android usage and links to documentation.

You might also be interested in lodepng_

.. _wiki: https://github.com/julienr/libpng-android/wiki
.. _lodepng: https://github.com/lvandeve/lodepng
