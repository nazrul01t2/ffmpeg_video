package com.example.ffmpegplayer

import android.view.Surface

object FfmpegBridge {
    init {
        System.loadLibrary("ffmpeg_player")
    }

    external fun prepare(path: String, surface: Surface): Boolean
    external fun play(): Boolean
    external fun pause(): Boolean
    external fun release()
    external fun seekTo(positionMs: Long)
    external fun setPlaybackSpeed(speed: Float)
    external fun selectSubtitleStream(streamIndex: Int)
    external fun getDurationMs(): Long
    external fun getCurrentPositionMs(): Long
    external fun setAvSyncToleranceMs(value: Int)
    external fun getSubtitleStreams(): Array<String>
}
