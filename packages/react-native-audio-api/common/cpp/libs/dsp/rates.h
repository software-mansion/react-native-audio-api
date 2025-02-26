#include "./common.h"

#ifndef SIGNALSMITH_DSP_RATES_H
#define SIGNALSMITH_DSP_RATES_H

#include "./windows.h"
#include "./delay.h"

namespace signalsmith {
namespace rates {
	/**	@defgroup Rates Multi-rate processing
		@brief Classes for oversampling/upsampling/downsampling etc.

		@{
		@file
	*/
	
	/// @brief Fills a container with a Kaiser-windowed sinc for an FIR lowpass.
	/// \diagram{rates-kaiser-sinc.svg,33-point results for various pass/stop frequencies}
	template<class Data>
	void fillKaiserSinc(Data &&data, int length, double passFreq, double stopFreq) {
		if (length <= 0) return;
		double kaiserBandwidth = (stopFreq - passFreq)*length;
		kaiserBandwidth += 1.25/kaiserBandwidth; // heuristic for transition band, see `InterpolatorKaiserSincN`
		auto kaiser = signalsmith::windows::Kaiser::withBandwidth(kaiserBandwidth);
		kaiser.fill(data, length);

		double centreIndex = (length - 1)*0.5;
		double sincScale = M_PI*(passFreq + stopFreq);
		double ampScale = (passFreq + stopFreq);
		for (int i = 0; i < length; ++i) {
			double x = (i - centreIndex), px = x*sincScale;
			double sinc = (std::abs(px) > 1e-6) ? std::sin(px)*ampScale/px : ampScale;
			data[i] *= sinc;
		}
	};
	/// @brief If only the centre frequency is specified, a heuristic is used to balance ripples and transition width
	/// \diagram{rates-kaiser-sinc-heuristic.svg,The transition width is set to: 0.9/sqrt(length)}
	template<class Data>
	void fillKaiserSinc(Data &&data, int length, double centreFreq) {
		double halfWidth = 0.45/std::sqrt(length);
		if (halfWidth > centreFreq) halfWidth = (halfWidth + centreFreq)*0.5;
		fillKaiserSinc(data, length, centreFreq - halfWidth, centreFreq + halfWidth);
	}

	/** 2x FIR oversampling for block-based processing.
 
		\diagram{rates-oversampler2xfir-responses-45.svg,Upsample response for various lengths}
	
		The oversampled signal is stored inside this object, with channels accessed via `oversampler[c]`.  For example, you might do:
		\code{.cpp}
			// Upsample from multi-channel input (inputBuffers[c][i] is a sample)
			oversampler.up(inputBuffers, bufferLength)
			
			// Modify the contents at the higher rate
			for (int c = 0; c < 2; ++c) {
				float *channel = oversampler[c];
				for (int i = 0; i < bufferLength*2; ++i) {
					channel[i] = std::abs(channel[i]);
				}
			}
			
			// Downsample into the multi-channel output
			oversampler.down(outputBuffers, bufferLength);
		\endcode

		The performance depends not just on the length, but also on where you end the passband, allowing a wider/narrower transition band.  Frequencies above this limit (relative to the lower sample-rate) may alias when upsampling and downsampling.

		\diagram{rates-oversampler2xfir-lengths.svg,Resample error rates for different passband thresholds}
	
		Since both upsample and downsample are stateful, channels are meaningful.  If your input channel-count doesn't match your output, you can size it to the larger of the two, and use `.upChannel()` and `.downChannel()` to only process the channels which exist.*/
	template<typename Sample>
	struct Oversampler2xFIR {
		Oversampler2xFIR() : Oversampler2xFIR(0, 0) {}
		Oversampler2xFIR(int channels, int maxBlock, int halfLatency=16, double passFreq=0.43) {
			resize(channels, maxBlock, halfLatency, passFreq);
		}
		
		void resize(int nChannels, int maxBlockLength) {
			resize(nChannels, maxBlockLength, oneWayLatency);
		}
		void resize(int nChannels, int maxBlockLength, int halfLatency, double passFreq=0.43) {
			oneWayLatency = halfLatency;
			kernelLength = oneWayLatency*2;
			channels = nChannels;
			halfSampleKernel.resize(kernelLength);
			fillKaiserSinc(halfSampleKernel, kernelLength, passFreq, 1 - passFreq);
			inputStride = kernelLength + maxBlockLength;
			inputBuffer.resize(channels*inputStride);
			stride = (maxBlockLength + kernelLength)*2;
			buffer.resize(stride*channels);
		}

		void reset() {
			inputBuffer.assign(inputBuffer.size(), 0);
			buffer.assign(buffer.size(), 0);
		}

		/// @brief Round-trip latency (or equivalently: upsample latency at the higher rate).
		/// This will be twice the value passed into the constructor or `.resize()`.
		int latency() const {
			return kernelLength;
		}
		
		/// Upsamples from a multi-channel input into the internal buffer
		template<class Data>
		void up(Data &&data, int lowSamples) {
			for (int c = 0; c < channels; ++c) {
				upChannel(c, data[c], lowSamples);
			}
		}

		/// Upsamples a single-channel input into the internal buffer
		template<class Data>
		void upChannel(int c, Data &&data, int lowSamples) {
			Sample *inputChannel = inputBuffer.data() + c*inputStride;
			for (int i = 0; i < lowSamples; ++i) {
				inputChannel[kernelLength + i] = data[i];
			}
			Sample *output = (*this)[c];
			for (int i = 0; i < lowSamples; ++i) {
				output[2*i] = inputChannel[i + oneWayLatency];
				Sample *offsetInput = inputChannel + (i + 1);
				Sample sum = 0;
				for (int o = 0; o < kernelLength; ++o) {
					sum += offsetInput[o]*halfSampleKernel[o];
				}
				output[2*i + 1] = sum;
			}
			// Copy the end of the buffer back to the beginning
			for (int i = 0; i < kernelLength; ++i) {
				inputChannel[i] = inputChannel[lowSamples + i];
			}
		}

		/// Downsamples from the internal buffer to a multi-channel output
		template<class Data>
		void down(Data &&data, int lowSamples) {
			for (int c = 0; c < channels; ++c) {
				downChannel(c, data[c], lowSamples);
			}
		}

		/// Downsamples a single channel from the internal buffer to a single-channel output
		template<class Data>
		void downChannel(int c, Data &&data, int lowSamples) {
			Sample *input = buffer.data() + c*stride; // no offset for latency
			for (int i = 0; i < lowSamples; ++i) {
				Sample v1 = input[2*i + kernelLength];
				Sample sum = 0;
				for (int o = 0; o < kernelLength; ++o) {
					Sample v2 = input[2*(i + o) + 1];
					sum += v2*halfSampleKernel[o];
				}
				Sample v2 = sum;
				Sample v = (v1 + v2)*Sample(0.5);
				data[i] = v;
			}
			// Copy the end of the buffer back to the beginning
			for (int i = 0; i < kernelLength*2; ++i) {
				input[i] = input[lowSamples*2 + i];
			}
		}

		/// Gets the samples for a single (higher-rate) channel.  The valid length depends how many input samples were passed into `.up()`/`.upChannel()`.
		Sample * operator[](int c) {
			return buffer.data() + kernelLength*2 + stride*c;
		}
		const Sample * operator[](int c) const {
			return buffer.data() + kernelLength*2 + stride*c;
		}

	private:
		int oneWayLatency, kernelLength;
		int channels;
		int stride, inputStride;
		std::vector<Sample> inputBuffer;
		std::vector<Sample> halfSampleKernel;
		std::vector<Sample> buffer;
	};

/** @} */
}} // namespace
#endif // include guard
