package com.swmansion.audioapi.system;

import android.content.Context;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.os.Build;

import com.facebook.react.bridge.ReactApplicationContext;

public class MusicControlAudioFocusListener implements AudioManager.OnAudioFocusChangeListener {
    private final MusicControlEventEmitter emitter;
    private final MusicControlVolumeListener volume;

    private AudioManager mAudioManager;
    private AudioFocusRequest mFocusRequest;

    private boolean mPlayOnAudioFocus = false;

    MusicControlAudioFocusListener(ReactApplicationContext context, MusicControlEventEmitter emitter,
                                   MusicControlVolumeListener volume) {
        this.emitter = emitter;
        this.volume = volume;

        this.mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    }

    @Override
    public void onAudioFocusChange(int focusChange) {
        if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
            abandonAudioFocus();
            mPlayOnAudioFocus = false;
            emitter.onStop();
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
            if (MusicControlModule.INSTANCE.isPlaying()) {
                mPlayOnAudioFocus = true;
                emitter.onPause();
            }
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) {
            volume.setCurrentVolume(40);
        } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
            if (volume.getCurrentVolume() != 100) {
                volume.setCurrentVolume(100);
            }
            if (mPlayOnAudioFocus) {
                emitter.onPlay();
            }
            mPlayOnAudioFocus = false;
        }
    }

    public void requestAudioFocus() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mFocusRequest = new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                    .setOnAudioFocusChangeListener(this).build();

            mAudioManager.requestAudioFocus(mFocusRequest);
        } else {
            mAudioManager.requestAudioFocus(this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        }
    }

    public void abandonAudioFocus() {
         if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && mAudioManager != null && mFocusRequest != null) {
            mAudioManager.abandonAudioFocusRequest(mFocusRequest);
        } else if ( mAudioManager != null ) {
            mAudioManager.abandonAudioFocus(this);
        }
    }
}
