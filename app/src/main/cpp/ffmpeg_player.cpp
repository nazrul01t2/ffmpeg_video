#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegPlayer", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegPlayer", __VA_ARGS__)

struct PlayerContext {
    AVFormatContext* formatContext = nullptr;
    ANativeWindow* nativeWindow = nullptr;
    std::string inputPath;
    int subtitleStream = -1;
    float playbackSpeed = 1.0f;
    int avSyncToleranceMs = 40;
    int64_t currentPositionMs = 0;
    bool playing = false;
};

static PlayerContext gPlayer;
static std::mutex gMutex;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_prepare(JNIEnv* env, jobject, jstring path, jobject surface) {
    std::lock_guard<std::mutex> lock(gMutex);

    const char* cPath = env->GetStringUTFChars(path, nullptr);
    gPlayer.inputPath = cPath;
    env->ReleaseStringUTFChars(path, cPath);

    if (gPlayer.formatContext) {
        avformat_close_input(&gPlayer.formatContext);
    }

    if (avformat_open_input(&gPlayer.formatContext, gPlayer.inputPath.c_str(), nullptr, nullptr) < 0) {
        LOGE("Failed to open: %s", gPlayer.inputPath.c_str());
        return JNI_FALSE;
    }

    if (avformat_find_stream_info(gPlayer.formatContext, nullptr) < 0) {
        LOGE("Failed stream info");
        return JNI_FALSE;
    }

    if (gPlayer.nativeWindow) {
        ANativeWindow_release(gPlayer.nativeWindow);
    }
    gPlayer.nativeWindow = ANativeWindow_fromSurface(env, surface);

    gPlayer.currentPositionMs = 0;
    gPlayer.playing = false;
    LOGI("Prepared media");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_play(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    if (!gPlayer.formatContext) return JNI_FALSE;
    gPlayer.playing = true;
    // Decoding/rendering threads should be started here.
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_pause(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    gPlayer.playing = false;
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_release(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    gPlayer.playing = false;
    if (gPlayer.nativeWindow) {
        ANativeWindow_release(gPlayer.nativeWindow);
        gPlayer.nativeWindow = nullptr;
    }
    if (gPlayer.formatContext) {
        avformat_close_input(&gPlayer.formatContext);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_seekTo(JNIEnv*, jobject, jlong positionMs) {
    std::lock_guard<std::mutex> lock(gMutex);
    if (!gPlayer.formatContext) return;

    int64_t targetUs = positionMs * 1000;
    av_seek_frame(gPlayer.formatContext, -1, targetUs, AVSEEK_FLAG_BACKWARD);
    gPlayer.currentPositionMs = positionMs;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_setPlaybackSpeed(JNIEnv*, jobject, jfloat speed) {
    std::lock_guard<std::mutex> lock(gMutex);
    gPlayer.playbackSpeed = speed;
    // Apply speed in audio time-stretch and video clock scheduling loops.
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_selectSubtitleStream(JNIEnv*, jobject, jint streamIndex) {
    std::lock_guard<std::mutex> lock(gMutex);
    gPlayer.subtitleStream = streamIndex;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_getDurationMs(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    if (!gPlayer.formatContext || gPlayer.formatContext->duration <= 0) return 0;
    return gPlayer.formatContext->duration / 1000;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_getCurrentPositionMs(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    return gPlayer.currentPositionMs;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_setAvSyncToleranceMs(JNIEnv*, jobject, jint value) {
    std::lock_guard<std::mutex> lock(gMutex);
    gPlayer.avSyncToleranceMs = value;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_ffmpegplayer_FfmpegBridge_getSubtitleStreams(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);

    std::vector<std::string> subtitles;
    if (gPlayer.formatContext) {
        for (unsigned int i = 0; i < gPlayer.formatContext->nb_streams; ++i) {
            const AVStream* stream = gPlayer.formatContext->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                subtitles.emplace_back("Subtitle stream #" + std::to_string(i));
            }
        }
    }

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(static_cast<jsize>(subtitles.size()), stringClass, nullptr);
    for (jsize i = 0; i < static_cast<jsize>(subtitles.size()); ++i) {
        env->SetObjectArrayElement(arr, i, env->NewStringUTF(subtitles[i].c_str()));
    }
    return arr;
}
