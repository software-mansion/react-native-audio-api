package com.swmansion.audioapi.system;

import android.content.Intent;
import android.os.Build;

import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;

import androidx.core.app.NotificationManagerCompat;

public class MusicControlEventEmitter {
    private static void sendEvent(ReactApplicationContext context, String type, Object value) {
        WritableMap data = Arguments.createMap();
        data.putString("name", type);

        if (value != null) {
            if (value instanceof Double || value instanceof Float) {
                data.putDouble("value", (double) value);
            } else if (value instanceof Boolean) {
                data.putBoolean("value", (boolean) value);
            } else if (value instanceof Integer) {
                data.putInt("value", (int) value);
            }
        }

        context.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class).emit("RNMusicControlEvent", data);
    }

    private final ReactApplicationContext context;
    private int notificationId;

    MusicControlEventEmitter(ReactApplicationContext context, int notificationId) {
        this.context = context;
        this.notificationId = notificationId;
    }

    public void onPlay() {
        sendEvent(context, "play", null);
    }

    public void onPause() {
        sendEvent(context, "pause", null);
    }

    public void onStop() {
        stopForegroundService();
        sendEvent(context, "stop", null);
    }

    public void onSkipToNext() {
        sendEvent(context, "nextTrack", null);
    }

    public void onSkipToPrevious() {
        sendEvent(context, "previousTrack", null);
    }

    public void onSeekTo(long pos) {
        sendEvent(context, "seek", pos / 1000D);
    }

    public void onFastForward() {
        sendEvent(context, "skipForward", null);
    }

    public void onRewind() {
        sendEvent(context, "skipBackward", null);
    }

    public void onSetRating(float rating) {
        sendEvent(context, "setRating", rating);
    }

    public void onSetRating(boolean hasHeartOrThumb) {
        sendEvent(context, "setRating", hasHeartOrThumb);
    }

    public void onVolumeChange(int volume) {
        sendEvent(context, "volume", volume);
    }

    private void stopForegroundService() {
        NotificationManagerCompat.from(context).cancel(notificationId);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Intent myIntent =
                    new Intent(context, MusicControlNotification.NotificationService.class);
            context.stopService(myIntent);
        }
    }
}
