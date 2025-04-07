package com.swmansion.audioapi.system;

import android.support.v4.media.RatingCompat;
import android.support.v4.media.session.MediaSessionCompat;

public class MediaSessionCallback extends MediaSessionCompat.Callback {
    private final MusicControlEventEmitter emitter;

    MediaSessionCallback(MusicControlEventEmitter emitter) {
        this.emitter = emitter;
    }

    @Override
    public void onPlay() {
        emitter.onPlay();
    }

    @Override
    public void onPause() {
        emitter.onPause();
    }

    @Override
    public void onStop() {
        emitter.onStop();
    }

    @Override
    public void onSkipToNext() {
        emitter.onSkipToNext();
    }

    @Override
    public void onSkipToPrevious() {
        emitter.onSkipToPrevious();
    }

    @Override
    public void onSeekTo(long pos) {
        emitter.onSeekTo(pos);
    }

    @Override
    public void onFastForward() {
        emitter.onFastForward();
    }

    @Override
    public void onRewind() {
        emitter.onRewind();
    }

    @Override
    public void onSetRating(RatingCompat rating) {
        if(MusicControlModule.INSTANCE == null) return;
        int type = MusicControlModule.INSTANCE.ratingType;

        if(type == RatingCompat.RATING_PERCENTAGE) {
            emitter.onSetRating(rating.getPercentRating());
        } else if(type == RatingCompat.RATING_HEART) {
            emitter.onSetRating(rating.hasHeart());
        } else if(type == RatingCompat.RATING_THUMB_UP_DOWN) {
            emitter.onSetRating(rating.isThumbUp());
        } else {
            emitter.onSetRating(rating.getStarRating());
        }
    }
}
