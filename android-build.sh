#!/bin/bash
# Copyright (C) 2012-2015 Jacob R. Lifshay
# This file is part of Voxels.
#
# Voxels is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Voxels is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Voxels; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
#
#

if [[ "$*" != "--release" && "$*" != "" ]]; then
    echo "usage: $0 [--release]"
    exit
fi

DEBUG_VERSION=1

if [[ "$*" == "--release" ]]; then
    DEBUG_VERSION=0
fi

PROJECT_PATH="`dirname "$0"`"
pushd "$PROJECT_PATH" > /dev/null
PROJECT_PATH="`pwd`"
popd > /dev/null
BUILD_PATH="$PROJECT_PATH/build"
rm -rf "$BUILD_PATH"
EXTERNAL_PATH="$PROJECT_PATH/external-lib/android"
SDL_PATH="$EXTERNAL_PATH/SDL2-2.0.3"
LIBPNG_PATH="$EXTERNAL_PATH/libpng-1.6.18"
LIBOGG_PATH="$EXTERNAL_PATH/libogg-1.3.2"
LIBVORBIS_PATH="$EXTERNAL_PATH/libvorbis-1.3.5"
OPENSSL_PATH="$EXTERNAL_PATH/android-external-openssl"
RESOURCES_PATH="$PROJECT_PATH/res"
NCPUS="1"
case "$OSTYPE" in
    darwin*)
        NCPUS="`sysctl -n hw.ncpu`"
        ;; 
    linux*)
        if [[ -n "`which nproc`" ]]; then
            NCPUS="`nproc`"
        fi
        ;;
    *);;
esac

function make_lib()
{
    local LIB_NAME="$1"
    local LIB_PATH="$2"
    local LOCAL_SHARED_LIBRARIES="$3"
    local SOURCE_DIRS="$4"
    local INCLUDE_DIRS="$5"
    local i
    mkdir -p "$BUILD_PATH/jni/$LIB_NAME"
    cp -r "$LIB_PATH" "$BUILD_PATH/jni/$LIB_NAME/src"
    cat > "$BUILD_PATH/jni/$LIB_NAME/Android.mk" <<EOF
LOCAL_PATH := \$(call my-dir)

include \$(CLEAR_VARS)

LOCAL_MODULE := ${LIB_NAME}

LOCAL_C_INCLUDES := \$(LOCAL_PATH)/src
EOF
    for i in $INCLUDE_DIRS; do
        i="/${i#/}"
        i="${i%/}"
        cat >> "$BUILD_PATH/jni/$LIB_NAME/Android.mk" <<EOF
LOCAL_C_INCLUDES += \$(LOCAL_PATH)$i
EOF
    done
    cat >> "$BUILD_PATH/jni/$LIB_NAME/Android.mk" <<EOF

LOCAL_SRC_FILES := \$(subst \$(LOCAL_PATH)/,, \\
EOF
    for i in $SOURCE_DIRS '/src'; do
        i="/${i#/}"
        i="${i%/}/"
        cat >> "$BUILD_PATH/jni/$LIB_NAME/Android.mk" <<EOF
        \$(wildcard \$(LOCAL_PATH)$i*.cpp) \\
        \$(wildcard \$(LOCAL_PATH)$i*.c) \\
EOF
    done
    cat >> "$BUILD_PATH/jni/$LIB_NAME/Android.mk" <<EOF
        )

LOCAL_CPP_FEATURES := rtti exceptions

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES

LOCAL_CPPFLAGS += -std=c++11

LOCAL_SHARED_LIBRARIES := ${LOCAL_SHARED_LIBRARIES}

#LOCAL_LDLIBS += -lz

include \$(BUILD_STATIC_LIBRARY)
EOF
}

cp -r "$SDL_PATH/android-project" "$BUILD_PATH"
mkdir -p "$BUILD_PATH/jni/SDL"
cp -r "$SDL_PATH/src" "$BUILD_PATH/jni/SDL"
cp -r "$SDL_PATH/include" "$BUILD_PATH/jni/SDL"
ln -s "$BUILD_PATH/jni/SDL/include" "$BUILD_PATH/jni/SDL/include/SDL2"
cp -r "$PROJECT_PATH/src" "$BUILD_PATH/jni/src"
cp -r "$PROJECT_PATH/include" "$BUILD_PATH/jni/src"
cp -r "$SDL_PATH/Android.mk" "$BUILD_PATH/jni/SDL"
make_lib "png" "$LIBPNG_PATH" "" "" "/"
ln -s "$BUILD_PATH/jni/png/src/scripts/pnglibconf.h.prebuilt" "$BUILD_PATH/jni/png/src/pnglibconf.h"
make_lib "ogg" "$LIBOGG_PATH" "" "/src/src" "src/include"
cat > "$BUILD_PATH/jni/ogg/src/include/ogg/config_types.h" <<'EOF'
#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#include <stdint.h>

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;

#endif
EOF
mkdir -p "$BUILD_PATH/jni/vorbis"
cp -r "$LIBVORBIS_PATH" "$BUILD_PATH/jni/vorbis/src"
cat > "$BUILD_PATH/jni/vorbis/Android.mk" <<'EOF'
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := vorbis

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../ogg/src/include

LOCAL_SRC_FILES := src/lib/mdct.c \
                   src/lib/smallft.c \
                   src/lib/block.c \
                   src/lib/envelope.c \
                   src/lib/window.c \
                   src/lib/lsp.c \
                   src/lib/lpc.c \
                   src/lib/analysis.c \
                   src/lib/synthesis.c \
                   src/lib/psy.c \
                   src/lib/info.c \
                   src/lib/floor1.c \
                   src/lib/floor0.c \
                   src/lib/res0.c \
                   src/lib/mapping0.c \
                   src/lib/registry.c \
                   src/lib/codebook.c \
                   src/lib/sharedbook.c \
                   src/lib/lookup.c \
                   src/lib/bitrate.c \
                   src/lib/vorbisfile.c

LOCAL_CPP_FEATURES := rtti exceptions

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES

LOCAL_CPPFLAGS += -std=c++11

LOCAL_STATIC_LIBRARIES := ogg

include $(BUILD_STATIC_LIBRARY)
EOF
#cp -r "$OPENSSL_PATH" "$BUILD_PATH/jni/openssl"
cat > "$BUILD_PATH/AndroidManifest.xml" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
      package="programmerjake.voxels"
      android:versionCode="1"
      android:versionName="0.7.4.2"
      android:installLocation="auto">

    <application android:label="@string/app_name"
                 android:icon="@drawable/ic_launcher"
                 android:allowBackup="true"
                 android:theme="@android:style/Theme.NoTitleBar.Fullscreen"
EOF
if ((DEBUG_VERSION)); then
    cat >> "$BUILD_PATH/AndroidManifest.xml" <<'EOF'
                 android:debuggable="true"
EOF
fi
cat >> "$BUILD_PATH/AndroidManifest.xml" <<'EOF'
                 android:hardwareAccelerated="true" >
        <activity android:name="MyActivity"
                  android:label="@string/app_name"
                  android:configChanges="keyboardHidden|orientation"
                  >
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>

    <!-- OpenGL ES 1.1 -->
    <uses-feature android:glEsVersion="0x00010001" />

    <!-- Android 4.1.1 -->
EOF
if ((DEBUG_VERSION)); then
    cat >> "$BUILD_PATH/AndroidManifest.xml" <<'EOF'
    <uses-sdk android:minSdkVersion="16" android:targetSdkVersion="16" />
EOF
else
    cat >> "$BUILD_PATH/AndroidManifest.xml" <<'EOF'
    <uses-sdk android:minSdkVersion="10" android:targetSdkVersion="16" />
EOF
fi
cat >> "$BUILD_PATH/AndroidManifest.xml" <<'EOF'

    <!-- Allow writing to external storage -->
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
</manifest>
EOF
cat > "$BUILD_PATH/jni/Application.mk" <<'EOF'
APP_STL := gnustl_shared

APP_ABI := armeabi-v7a armeabi x86
EOF
cat > "$BUILD_PATH/jni/src/Android.mk" <<'EOF'
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
LIBPNG_PATH := ../png
LIBOGG_PATH := ../ogg
LIBVORBIS_PATH := ../vorbis
OPENSSL_PATH := ../openssl

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBPNG_PATH)/src
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBOGG_PATH)/src/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBVORBIS_PATH)/src/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(OPENSSL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
        $(subst $(LOCAL_PATH)/,, \
        $(wildcard $(LOCAL_PATH)/src/*/*.cpp) \
        $(wildcard $(LOCAL_PATH)/src/*/*.c) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*.cpp) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*.c) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*/*.cpp) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*/*.c) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*/*/*.cpp) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*/*/*.c) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*/*/*/*.cpp) \
        $(wildcard $(LOCAL_PATH)/src/*/*/*/*/*/*.c) \
        )

LOCAL_CPP_FEATURES := rtti exceptions

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DDEBUG_VERSION

LOCAL_CPPFLAGS += -std=c++11 -Dthread_local=thread_local_error

LOCAL_SHARED_LIBRARIES := SDL2

#LOCAL_STATIC_LIBRARIES := openssl-static

LOCAL_STATIC_LIBRARIES += png ogg vorbis

LOCAL_LDLIBS += -lz -lGLESv1_CM

include $(BUILD_SHARED_LIBRARY)
EOF
mkdir -p "$BUILD_PATH/src/programmerjake/voxels"
cat > "$BUILD_PATH/src/programmerjake/voxels/MyActivity.java" <<'EOF'
package programmerjake.voxels;
import org.libsdl.app.SDLActivity;

public class MyActivity extends SDLActivity {
    static {
        System.loadLibrary("gnustl_shared");
        //System.loadLibrary("png");
        //System.loadLibrary("ogg");
        //System.loadLibrary("vorbis");
        System.loadLibrary("SDL2");
        System.loadLibrary("main");
    }
}
EOF
sed -i '{s/System\.loadLibrary("SDL2");/\/\/System.loadLibrary("SDL2");/g; s/System\.loadLibrary("main");/\/\/System.loadLibrary("main");/g}' "$BUILD_PATH/src/org/libsdl/app/SDLActivity.java"
sed -i '{s/SDLActivity/Voxels-0.7/g}' "$BUILD_PATH/build.xml"
sed -i '{s/<string name="app_name">SDL App<\/string>/<string name="app_name">Voxels 0.7<\/string>/g}' "$BUILD_PATH/res/values/strings.xml"
mkdir -p "$BUILD_PATH/assets"
ln -sT "$RESOURCES_PATH" "$BUILD_PATH/assets/res" 
android update project --path "$BUILD_PATH" --target android-16
pushd "$BUILD_PATH" > /dev/null
if ((DEBUG_VERSION)); then
    ndk-build -j"$NCPUS" && ant debug
else
    ndk-build -j"$NCPUS" && ant debug
    printf "\x1b[;31;1mImportant: sign and zip-align the resulting package!\x1b[m\n"
fi
popd > /dev/null
