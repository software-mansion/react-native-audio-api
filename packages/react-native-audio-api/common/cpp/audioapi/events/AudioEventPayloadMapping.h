#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>

#include <concepts>
#include <type_traits>

namespace audioapi {

template <AudioEvent Event>
struct AllowedEventPayload;

#define AUDIOAPI_DEFINE_EVENT_PAYLOAD(Event, Payload) \
  template <> \
  struct AllowedEventPayload<Event> { \
    using type = Payload; \
  }

AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_PLAY, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_PAUSE, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_STOP, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_NEXT_TRACK, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_PREVIOUS_TRACK, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_SKIP_FORWARD, DoubleValuePayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_SKIP_BACKWARD, DoubleValuePayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_SEEK_FORWARD, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_SEEK_BACKWARD, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_SEEK_TO, DoubleValuePayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::PLAYBACK_NOTIFICATION_DISMISSED, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::RECORDING_NOTIFICATION_RESUME, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::RECORDING_NOTIFICATION_PAUSE, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::ROUTE_CHANGE, StringPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::INTERRUPTION, InterruptionPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::VOLUME_CHANGE, DoubleValuePayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::DUCK, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::ENDED, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::LOOP_ENDED, EmptyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::AUDIO_READY, AudioReadyPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::POSITION_CHANGED, DoubleValuePayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::BUFFER_ENDED, BufferEndedPayload);
AUDIOAPI_DEFINE_EVENT_PAYLOAD(AudioEvent::RECORDER_ERROR, StringPayload);

#undef AUDIOAPI_DEFINE_EVENT_PAYLOAD

template <AudioEvent Event>
using EventPayloadType = AllowedEventPayload<Event>::type;

template <AudioEvent Event, typename Payload>
concept EventPayloadFor = std::same_as<std::remove_cvref_t<Payload>, EventPayloadType<Event>>;

} // namespace audioapi
