package com.swmansion.audioapi.system;

import androidx.media.VolumeProviderCompat;
import com.facebook.react.bridge.ReactApplicationContext;

public class MusicControlVolumeListener extends VolumeProviderCompat {
    private final ReactApplicationContext context;
    private final MusicControlEventEmitter emitter;

    MusicControlVolumeListener(ReactApplicationContext context, MusicControlEventEmitter emitter, boolean changeable, int maxVolume, int currentVolume) {
        super(changeable ? VOLUME_CONTROL_ABSOLUTE : VOLUME_CONTROL_FIXED, maxVolume, currentVolume);
        this.context = context;
        this.emitter = emitter;
    }

    public boolean isChangeable() {
        return getVolumeControl() != VolumeProviderCompat.VOLUME_CONTROL_FIXED;
    }

    @Override
    public void onSetVolumeTo(int volume) {
        setCurrentVolume(volume);
        emitter.onVolumeChange(volume);
    }

    @Override
    public void onAdjustVolume(int direction) {
        int maxVolume = getMaxVolume();
        int tick = direction * (maxVolume / 10);
        int volume = Math.max(Math.min(getCurrentVolume() + tick, maxVolume), 0);

        setCurrentVolume(volume);
        emitter.onVolumeChange(volume);
    }

    public MusicControlVolumeListener create(Boolean changeable, Integer maxVolume, Integer currentVolume) {
        if(currentVolume == null) {
            currentVolume = getCurrentVolume();
        } else {
            setCurrentVolume(currentVolume);
        }

        if(changeable == null) changeable = isChangeable();
        if(maxVolume == null) maxVolume = getMaxVolume();

        if(changeable == isChangeable() && maxVolume == getMaxVolume()) return this;
        return new MusicControlVolumeListener(context, emitter, changeable, maxVolume, currentVolume);
    }
}
