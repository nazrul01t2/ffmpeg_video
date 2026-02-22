# FFmpeg JNI Kotlin Video Player

Android sample that wires a Kotlin UI to a native FFmpeg player core through JNI.

## Features included
- Playback controls (play / pause)
- Playback speed selection (`0.5x` to `2.0x`)
- Seek bar and periodic progress updates
- Subtitle stream selection spinner
- AV sync tolerance setter (`setAvSyncToleranceMs`)

## Architecture
- `MainActivity.kt`: UI, progress loop, speed/subtitle controls
- `FfmpegBridge.kt`: JNI boundary
- `ffmpeg_player.cpp`: FFmpeg C-API based native context and control functions

## Where do I get the FFmpeg files?
You need two things:
1. FFmpeg headers (`include/`)
2. FFmpeg shared libs (`libavcodec.so`, `libavformat.so`, `libavutil.so`, `libswresample.so`, `libswscale.so`) for each ABI.

### Option A (recommended): Build FFmpeg locally with the included script
1. Install Android NDK (for example from Android Studio SDK Manager).
2. Set your NDK path:
   ```bash
   export ANDROID_NDK_HOME=$HOME/Android/Sdk/ndk/26.3.11579264
   ```
3. Run:
   ```bash
   ./scripts/build_ffmpeg_android.sh
   ```

The script clones FFmpeg source from the official GitHub mirror and builds these ABIs:
- `arm64-v8a`
- `armeabi-v7a`
- `x86_64`

Output is copied to:
- `app/src/main/cpp/ffmpeg/<abi>/lib*.so`
- `app/src/main/cpp/ffmpeg/include`

### Option B: Use your own prebuilt FFmpeg binaries
If you already have prebuilt `.so` files, copy them manually into:
- `app/src/main/cpp/ffmpeg/arm64-v8a/`
- `app/src/main/cpp/ffmpeg/armeabi-v7a/`
- `app/src/main/cpp/ffmpeg/x86_64/`

And copy FFmpeg headers into:
- `app/src/main/cpp/ffmpeg/include`

## Build app
After FFmpeg files are in place:
```bash
./gradlew assembleDebug
```

## Important notes
- Replace sample input path in `MainActivity.surfaceCreated()` with your media picker / SAF URI flow.
- Current native code is a scaffold. It opens media and exposes controls, but full decode/render/audio pipeline still needs implementation.

## Next implementation steps in native side
- Add decode threads (video/audio/subtitle)
- Render video frames to `ANativeWindow`
- Audio output via `AudioTrack` or AAudio bridge
- Real clock-based AV sync loop using `avSyncToleranceMs`
- Subtitle parser/render overlay path
