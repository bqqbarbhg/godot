#include "speech_decoder.h"
#include "speech_processor.h"

int32_t SpeechDecoder::process(const PackedByteArray *p_compressed_buffer,
		PackedByteArray *p_pcm_output_buffer,
		const int p_compressed_buffer_size,
		const int p_pcm_output_buffer_size,
		const int p_buffer_frame_count) {
	*p_pcm_output_buffer->ptrw() = 0;
	if (!decoder) {
		return OPUS_INVALID_STATE;
	}
	opus_int16 *output_buffer_pointer =
			reinterpret_cast<opus_int16 *>(p_pcm_output_buffer->ptrw());
	const unsigned char *opus_buffer_pointer =
			reinterpret_cast<const unsigned char *>(p_compressed_buffer->ptr());

	opus_int32 ret_value =
			opus_decode(decoder, opus_buffer_pointer, p_compressed_buffer_size,
					output_buffer_pointer, p_buffer_frame_count, 0);
	return ret_value;
}

SpeechDecoder::SpeechDecoder() {
	int error = OPUS_INVALID_STATE;
	decoder = opus_decoder_create(
			SpeechProcessor::SPEECH_SETTING_SAMPLE_RATE, SpeechProcessor::SPEECH_SETTING_CHANNEL_COUNT, &error);
	if (error != OPUS_OK) {
		ERR_PRINT("OpusCodec: could not create Opus decoder!");
	}
}

SpeechDecoder::~SpeechDecoder() {
	opus_decoder_destroy(decoder);
}
