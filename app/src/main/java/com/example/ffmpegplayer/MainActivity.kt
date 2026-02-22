package com.example.ffmpegplayer

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.SurfaceHolder
import android.widget.ArrayAdapter
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import com.example.ffmpegplayer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {
    private lateinit var binding: ActivityMainBinding
    private val uiHandler = Handler(Looper.getMainLooper())
    private var playing = false
    private var userSeeking = false

    private val ticker = object : Runnable {
        override fun run() {
            if (!userSeeking) {
                val duration = FfmpegBridge.getDurationMs().coerceAtLeast(1L)
                val current = FfmpegBridge.getCurrentPositionMs().coerceAtLeast(0L)
                binding.progressBar.max = duration.toInt()
                binding.progressBar.progress = current.toInt()
                binding.timeLabel.text = "${toTime(current)} / ${toTime(duration)}"
            }
            uiHandler.postDelayed(this, 300)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.videoSurface.holder.addCallback(this)

        binding.playPauseBtn.setOnClickListener {
            playing = if (playing) {
                FfmpegBridge.pause()
                false
            } else {
                FfmpegBridge.play()
                true
            }
            binding.playPauseBtn.text = if (playing) "Pause" else "Play"
        }

        setupSpeedSpinner()
        setupSubtitleSpinner(emptyArray())

        binding.progressBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) = Unit
            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                userSeeking = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                userSeeking = false
                seekBar?.progress?.toLong()?.let { FfmpegBridge.seekTo(it) }
            }
        })

        uiHandler.post(ticker)
    }

    private fun setupSpeedSpinner() {
        val speedValues = listOf(0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f)
        binding.speedSpinner.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_dropdown_item,
            speedValues.map { "${it}x" }
        )
        binding.speedSpinner.setSelection(speedValues.indexOf(1.0f))
        binding.speedSpinner.setOnItemSelectedListener { _, _, position, _ ->
            FfmpegBridge.setPlaybackSpeed(speedValues[position])
        }
    }

    private fun setupSubtitleSpinner(streams: Array<String>) {
        val data = listOf("Off") + streams.toList()
        binding.subtitleSpinner.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_dropdown_item,
            data
        )
        binding.subtitleSpinner.setOnItemSelectedListener { _, _, position, _ ->
            FfmpegBridge.selectSubtitleStream(if (position == 0) -1 else position - 1)
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // Replace with your input source (local file/URI resolver)
        val source = "/sdcard/Movies/sample.mp4"
        if (FfmpegBridge.prepare(source, holder.surface)) {
            setupSubtitleSpinner(FfmpegBridge.getSubtitleStreams())
            FfmpegBridge.setAvSyncToleranceMs(40)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        FfmpegBridge.release()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) = Unit

    override fun onDestroy() {
        uiHandler.removeCallbacksAndMessages(null)
        super.onDestroy()
    }

    private fun toTime(ms: Long): String {
        val totalSeconds = ms / 1000
        val minutes = totalSeconds / 60
        val seconds = totalSeconds % 60
        return "%02d:%02d".format(minutes, seconds)
    }
}

private fun android.widget.Spinner.setOnItemSelectedListener(block: (android.widget.AdapterView<*>, android.view.View?, Int, Long) -> Unit) {
    onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
        override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: android.view.View?, position: Int, id: Long) {
            if (parent != null) block(parent, view, position, id)
        }

        override fun onNothingSelected(parent: android.widget.AdapterView<*>?) = Unit
    }
}
