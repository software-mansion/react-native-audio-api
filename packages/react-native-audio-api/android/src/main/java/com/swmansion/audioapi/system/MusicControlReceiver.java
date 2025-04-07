package com.swmansion.audioapi.system;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.view.KeyEvent;
import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;

public class MusicControlReceiver extends BroadcastReceiver {

    private final MusicControlModule module;
    private final String packageName;
    private final ReactApplicationContext reactContext;

    public MusicControlReceiver(MusicControlModule module, ReactApplicationContext context) {
        this.module = module;
        this.packageName = context.getPackageName();
        this.reactContext = context;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if(module.session == null || module.notification == null) return;
        String action = intent.getAction();

        if(MusicControlNotification.REMOVE_NOTIFICATION.equals(action)) {

            if(!checkApp(intent)) return;

            // Removes the notification and deactivates the media session
            module.notification.hide();
            module.session.setActive(false);

            // Notify react native
            WritableMap data = Arguments.createMap();
            data.putString("name", "closeNotification");
            reactContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class).emit("RNMusicControlEvent", data);

        } else if(MusicControlNotification.MEDIA_BUTTON.equals(action) || Intent.ACTION_MEDIA_BUTTON.equals(action)) {

            if(!intent.hasExtra(Intent.EXTRA_KEY_EVENT)) return;
            if(!checkApp(intent)) return;

            // Dispatch media buttons to MusicControlListener
            // Copy of MediaButtonReceiver.handleIntent without action check
            KeyEvent ke = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            module.session.getController().dispatchMediaButtonEvent(ke);

        } else if(AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(action)) {

            module.session.getController().getTransportControls().pause();

        }
    }

    private boolean checkApp(Intent intent) {
        if(intent.hasExtra(MusicControlNotification.PACKAGE_NAME)) {
            String name = intent.getStringExtra(MusicControlNotification.PACKAGE_NAME);
            if(!packageName.equals(name)) return false; // This event is not for this package. We'll ignore it
        }
        return true;
    }

}
