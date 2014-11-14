## Overview

I created this project, in order to learn more about game programming.
I had chosen Minecraft due to it's simple game-design, which enabled me to focus more on learning and less on the art of creating games (for now).

## Dependencies

* C++11 Compiler (e.g. Xcode 6, Visual Studio 2013, gcc, …)
* Python 2.6 or newer
* gyp

## Build Instructions

### OS X / Linux

    $ ./configure -f [cmake|eclipse|make|msvs|ninja|xcode]

E.g. for Xcode:

    $ ./configure -f xcode

If you don't wan't to, or can't use the configure script, please clone gyp in the build folder and use gyp_glcraft.py directly. It should look similiar to this:

    $ mkdir build
    $ git clone https://git.chromium.org/external/gyp.git build/gyp
    $ ./gyp_glcraft.py -f [cmake|eclipse|make|msvs|ninja|xcode]

## Unterstützte Plattformen

__At the time of writing only Xcode 6.1 on OS X 10.10 is fully tested.__

Windows and Linux and other build systems still need to be tested, but probably work instantly without, or with small changes tops.
