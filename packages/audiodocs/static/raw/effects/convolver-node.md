# ConvolverNode

The `ConvolverNode` interface represents a linear convolution effect, that can be applied to a signal given an impulse response.
This is the easiest way to achieve `echo` or [`reverb`](https://en.wikipedia.org/wiki/Reverb_effect) effects.

#### [`AudioNode`](/docs/core/audio-node#properties) properties

> **Info**
>
> Convolver is a node with tail-time, which means, that it continues to output non-silent audio with zero input for the length of the buffer.

## Constructor

```tsx
constructor(context: BaseAudioContext, options?: ConvolverOptions)
```

### `ConvolverOptions`

Inherits all properties from [`AudioNodeOptions`](/docs/core/audio-node#audionodeoptions)

| Parameter | Type | Default | |
| :---: | :---: | :----: | :---- |
| `buffer`  | `number` |  | Initial value for [`buffer`](/docs/effects/convolver-node#properties). |
| `normalize`  | `boolean` | true | Initial value for [`normalize`](/docs/effects/convolver-node#properties). |

Or by using `BaseAudioContext` factory method:
[`BaseAudioContext.createConvolver()`](/docs/core/base-audio-context#createconvolver)

## Properties

It inherits all properties from [`AudioNode`](/docs/core/audio-node#properties).

| Name | Type | Description |
| :----: | :----: | :-------- |
| `buffer` | [`AudioBuffer`](/docs/sources/audio-buffer) | Associated AudioBuffer. |
| `normalize` | `boolean` | Whether the impulse response from the buffer will be scaled by an equal-power normalization when the buffer attribute is set. |

> **Caution**
>
> Linear convolution is a heavy computational process, so if your audio has some weird artefacts that should not be there, try to decrease the duration of impulse response buffer.
