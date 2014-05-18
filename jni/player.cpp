#include <stdio.h>
#include <pthread.h>
#include <jni.h>
#include <unistd.h>

#include <map>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
}

#include "player.h"
#include "error.h"
#include "PlayerData.h"

#define LOG_TAG "Native"
#define VERSION "0.3.0"

std::map<jint, PlayerData*> playerMap;
JavaVM *jvm;

jboolean isBuffer(JNIEnv *pEnv, jobject pObj) {
	jclass cls = pEnv->GetObjectClass(pObj);
	jfieldID fid = pEnv->GetFieldID(cls, "isBuffer", "Z");
	jboolean result = pEnv->GetBooleanField(pObj, fid);
	pEnv->DeleteLocalRef(cls);
	return result;
}

jboolean isMute(JNIEnv *pEnv, jobject pObj) {
	jclass cls = pEnv->GetObjectClass(pObj);
	jfieldID fid = pEnv->GetFieldID(cls, "isMute", "Z");
	jboolean result = pEnv->GetBooleanField(pObj, fid);
	pEnv->DeleteLocalRef(cls);
	return result;
}

jboolean isVerbose(JNIEnv *pEnv, jobject pObj) {
	jclass cls = pEnv->GetObjectClass(pObj);
	jfieldID fid = pEnv->GetFieldID(cls, "isVerbose", "Z");
	jboolean result = pEnv->GetBooleanField(pObj, fid);
	pEnv->DeleteLocalRef(cls);
	return result;
}

void LOG(JNIEnv *env, jobject obj, int lv, const char *msg, va_list args) {
	if(isVerbose(env, obj)) {
		__android_log_vprint(lv, LOG_TAG, msg, args);
	}
}

void LOGD(JNIEnv *env, jobject obj, const char * msg, ...) {
	va_list args;
	va_start(args, msg);
	LOG(env, obj, 3, msg, args);
	va_end(args);
}

void LOGI(JNIEnv *env, jobject obj, const char * msg, ...) {
	va_list args;
	va_start(args, msg);
	LOG(env, obj, 4, msg, args);
	va_end(args);
}

void LOGE(JNIEnv *env, jobject obj, const char * msg, ...) {
	va_list args;
	va_start(args, msg);
	LOG(env, obj, 6, msg, args);
	va_end(args);
}

jint getHashCode(JNIEnv *env, jobject obj) {
	jclass cls = env->GetObjectClass(obj);
	jmethodID mid = env->GetMethodID(cls, "hashCode", "()I");
	if( mid == 0 ) {
		LOGE(env, obj, "cannot get hash code");
		return 0;
	}
	return env->CallIntMethod(obj, mid);
}

jstring getPath(JNIEnv *env, jobject obj) {
	jclass cls = env->GetObjectClass(obj);
	jfieldID fid = env->GetFieldID(cls, "mPath", "Ljava/lang/String;");
	if( fid == 0 ) {
		LOGE(env, obj, "cannot get mPath");
		return 0;
	}
	return (jstring) env->GetObjectField(obj, fid);
}

jobject getSurface(JNIEnv *env, jobject obj) {
	jclass cls = env->GetObjectClass(obj);
	jfieldID fid = env->GetFieldID(cls, "mSurface", "Landroid/view/Surface;");
	if( fid == 0 ) {
		LOGE(env, obj, "cannot get mSurface");
		return 0;
	}
	return (jobject) env->GetObjectField(obj, fid);
}

jboolean isPlay(JNIEnv *pEnv, jobject pObj) {
	jclass cls = pEnv->GetObjectClass(pObj);
	jfieldID fid = pEnv->GetFieldID(cls, "isPlay", "Z");
	jboolean result = pEnv->GetBooleanField(pObj, fid);
	pEnv->DeleteLocalRef(cls);
	return result;
}

PlayerData* getPlayerData(JNIEnv *pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	return playerMap.find(hashCode)->second;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naInit
(JNIEnv *pEnv, jobject pObj) {
	PlayerData *player;
	jint hashCode = getHashCode(pEnv, pObj);
	LOGD(pEnv, pObj, "naInit start");

	LOGI(pEnv, pObj, "NewLPlayer version %s", VERSION);

	if(jvm == NULL) {
		pEnv->GetJavaVM(&jvm);
	}

	if( playerMap.find(hashCode) != playerMap.end() ) {
		LOGE(pEnv, pObj, "this player object is already initialized");
		return ERROR_ALREADY_INITIALIZED;
	}
	else {
		player = new PlayerData(pEnv, pObj, isBuffer(pEnv, pObj));
		playerMap.insert(std::pair<jint, PlayerData*>(hashCode, player));
	}

	// Register all formats and codecs
	av_register_all();
	// if is network
	avformat_network_init();
	// Open video file

	LOGD(pEnv, pObj, "naInit done");
	return 0;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naSetSurface
(JNIEnv *pEnv, jobject pObj) {
	PlayerData *player	= getPlayerData(pEnv, pObj);
	jobject pSurface		= getSurface(pEnv, pObj);
	if (pSurface) {
		player->window = ANativeWindow_fromSurface(pEnv, pSurface);
	}
	ANativeWindow_setBuffersGeometry(player->window, player->videoCodecCtx->width, player->videoCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
}

JNIEXPORT jintArray JNICALL Java_com_misgood_newlplayer_Player_naGetVideoRes
(JNIEnv *pEnv, jobject pObj) {
	PlayerData *player = getPlayerData(pEnv, pObj);
	jintArray lRes;
	if (NULL == player->videoCodecCtx) {
		return NULL;
	}
	lRes = pEnv->NewIntArray(2);
	if (lRes == NULL) {
		LOGE(pEnv, pObj, "cannot allocate memory for video size");
		return NULL;
	}
	jint lVideoRes[2];
	lVideoRes[0] = player->videoCodecCtx->width;
	lVideoRes[1] = player->videoCodecCtx->height;
	pEnv->SetIntArrayRegion(lRes, 0, 2, lVideoRes);
	return lRes;
}

JNIEXPORT jdouble JNICALL Java_com_misgood_newlplayer_Player_naGetFps
(JNIEnv * pEnv, jobject pObj) {
	return getPlayerData(pEnv, pObj)->frameRate;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naGetSampleRate
(JNIEnv * pEnv, jobject pObj) {
	AVCodecContext *ctx = getPlayerData(pEnv, pObj)->audioCodecCtx;
	return ctx != NULL ? ctx->sample_rate : 0;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naGetChannels
(JNIEnv * pEnv, jobject pObj) {
	AVCodecContext *ctx = getPlayerData(pEnv, pObj)->audioCodecCtx;
	return ctx != NULL ? ctx->channels : 0;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naSetup
(JNIEnv * pEnv, jobject pObj) {
	PlayerData *player		= getPlayerData(pEnv, pObj);
	AVCodec *pVideoCodec	= NULL;
	AVCodec *pAudioCodec	= NULL;
	AVDictionary *opts		= 0;
	jstring pFileName			= getPath(pEnv, pObj);
	jobject pSurface			= getSurface(pEnv, pObj);

	LOGD(pEnv, pObj, "naSetup start");

	player->videoFileName = (char *) pEnv->GetStringUTFChars(pFileName, NULL);
	LOGI(pEnv, pObj, "video file name is %s", player->videoFileName);
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	if( avformat_open_input(&(player->formatCtx), player->videoFileName, NULL, &opts) != 0){
		LOGE(pEnv, pObj, "cannot open video.");
		return ERROR_CANT_OPEN_INPUT; // Couldn't open file
	}
	// Retrieve stream information
	if(avformat_find_stream_info(player->formatCtx, NULL)<0) {
		LOGE(pEnv, pObj, "cannot find stream information.");
		return ERROR_NO_STREAM_INFO_FOUND; // Couldn't find stream information
	}
	// Dump information about file onto standard error
	av_dump_format((player->formatCtx), 0, player->videoFileName, 0);
	av_dict_free(&opts);
	// Find best video stream
	player->videoStream = av_find_best_stream(player->formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if(player->videoStream == AVERROR_STREAM_NOT_FOUND) {
		LOGE(pEnv, pObj, "video stream not found.");
	}
	else if(player->videoStream == AVERROR_DECODER_NOT_FOUND) {
		LOGE(pEnv, pObj, "video stream is found, but no available decoder");
		return ERROR_NO_AVAILABLE_DECODER;
	}
	// Find best audio stream
	player->audioStream = av_find_best_stream(player->formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if(player->audioStream == AVERROR_STREAM_NOT_FOUND) {
		LOGI(pEnv, pObj, "audio stream not found.");
	}
	else if(player->audioStream == AVERROR_DECODER_NOT_FOUND) {
		LOGE(pEnv, pObj, "audio stream is found, but no available decoder");
		return ERROR_NO_AVAILABLE_DECODER;
	}
	if( player->audioStream < 0 && player->videoStream < 0 ) {
		LOGE(pEnv, pObj, "cannot find any media stream");
		return ERROR_STREAM_NOT_FOUND;
	}
	if( player->videoStream >= 0 ) {
		player->frameRate = av_q2d(player->formatCtx->streams[player->videoStream]->r_frame_rate);
		LOGI(pEnv, pObj, "frame rate: %f", player->frameRate);
		// Get a pointer to the codec context for the video stream
		player->videoCodecCtx = player->formatCtx->streams[player->videoStream]->codec;
		pVideoCodec = avcodec_find_decoder(player->videoCodecCtx->codec_id);
		if(pVideoCodec == NULL) {
			LOGE(pEnv, pObj, "unsupported codec");
			return -1; // Codec not found
		}
		// Open codec
		if(avcodec_open2(player->videoCodecCtx, pVideoCodec, NULL) < 0) {
			// Could not open codec
			LOGE(pEnv, pObj, "cannot open codec '%s'.", player->videoCodecCtx->codec_name);
			return ERROR_CANT_OPEN_DECODER;
		}
		player->frameRGBA = av_frame_alloc();
		if(player->frameRGBA == NULL) {
			LOGE(pEnv, pObj, "allocation fail.");
			return ERROR_FRAME_ALLOC_FAIL;
		}
		// set surface
		//setSurface(player, pEnv, pSurface);
		// set format and size of window buffer
		//ANativeWindow_setBuffersGeometry(player->window, player->videoCodecCtx->width, player->videoCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
		//get the scaling context
		if( !(player->swsCtx = sws_getCachedContext (
				NULL,
				player->videoCodecCtx->width,
				player->videoCodecCtx->height,
				player->videoCodecCtx->pix_fmt,
				player->videoCodecCtx->width,
				player->videoCodecCtx->height,
				AV_PIX_FMT_RGBA,
				SWS_X,
				NULL,
				NULL,
				NULL
		))) {
			LOGE(pEnv, pObj, "cannot get sws context");
			return ERROR_CANT_GET_SWS_CONTEXT;
		}
		player->avpictureSize = avpicture_get_size(AV_PIX_FMT_RGBA, player->videoCodecCtx->width, player->videoCodecCtx->height);
		player->buffer = (uint8_t*) av_malloc( player->avpictureSize * sizeof(uint8_t) );
		avpicture_fill((AVPicture *)player->frameRGBA, player->buffer, AV_PIX_FMT_RGBA, player->videoCodecCtx->width, player->videoCodecCtx->height);
	}
	if( player->audioStream >= 0 ) {
		player->audioCodecCtx = player->formatCtx->streams[player->audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(player->audioCodecCtx->codec_id);
		LOGI(pEnv, pObj, "audio format: %s", av_get_sample_fmt_name(player->audioCodecCtx->sample_fmt));
		if(pAudioCodec == NULL) {
			LOGE(pEnv, pObj, "unsupported codec");
			return -1; // Codec not found
		}
		if(avcodec_open2(player->audioCodecCtx, pAudioCodec, NULL) < 0) {
			LOGE(pEnv, pObj, "cannot open codec %s.", player->audioCodecCtx->codec_name);
			return ERROR_CANT_OPEN_DECODER;
		}
		if(player->audioCodecCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
			player->swrCtx = swr_alloc_set_opts(
					NULL,
					av_get_default_channel_layout(player->audioCodecCtx->channels),
					AV_SAMPLE_FMT_S16,
					player->audioCodecCtx->sample_rate,
					av_get_default_channel_layout(player->audioCodecCtx->channels),
					player->audioCodecCtx->sample_fmt,
					player->audioCodecCtx->sample_rate,
					0,
					NULL);
			if(!player->swrCtx) {
				LOGE(pEnv, pObj, "cannot allocate swr context");
				return ERROR_CANT_ALLOCATE_SWR_CONTEXT;
			}
			if( swr_init(player->swrCtx) < 0 ) {
				LOGE(pEnv, pObj, "cannot initialize swr context");
				return ERROR_INIT_SWR_CONTEXT_FAIL;
			}
		}
	}

	player->decodedFrame = av_frame_alloc();
	if(player->decodedFrame == NULL) {
		LOGE(pEnv, pObj, "allocation fail.");
		return ERROR_FRAME_ALLOC_FAIL;
	}
	LOGD(pEnv, pObj, "naSetup done");
	return 0;
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naDecode
(JNIEnv * pEnv, jobject pObj) {
	AVPacket packet;
	PlayerData *player			= getPlayerData(pEnv, pObj);
	int gotFrame				= 0;
	uint8_t **convertedData;
	int isSkip 					= 0;

	LOGD(pEnv, pObj, "naDecode start");

	int sumSize = 0;

	av_init_packet(&packet);
	while(isPlay(pEnv, pObj)) {
		if( av_read_frame(player->formatCtx, &packet) < 0 ) {
			LOGE(pEnv, pObj, "cannot read packet");
		}
		if(packet.stream_index == player->videoStream) {
			int readSize = avcodec_decode_video2(player->videoCodecCtx, player->decodedFrame, &gotFrame, &packet);
			if( !player->decodedFrame->key_frame && isSkip ) {
				continue;
			}
			else if( player->decodedFrame->key_frame && isSkip ) {
				isSkip = 0;
			}
			if( player->decodedFrame->is_corrupt ) {
				LOGD(pEnv, pObj, "decodeFrame is corrupt");
				isSkip = 1;
				continue;
			}
			if( readSize < 0) {
				LOGE(pEnv, pObj, "error decoding video frame");
			}
			else {
				sumSize += readSize;
			}
			if(gotFrame) {
				int expectSize = avpicture_get_size(player->videoCodecCtx->pix_fmt, player->videoCodecCtx->width, player->videoCodecCtx->height);
				int getSize = av_frame_get_pkt_size(player->decodedFrame);
				LOGD(pEnv, pObj, "(read, expect, get): (%d, %d, %d)",  sumSize, expectSize, getSize);
				sumSize = 0;
				// Convert the image from its native format to RGBA
				sws_scale(
						player->swsCtx,
						(uint8_t const * const *)player->decodedFrame->data,
						player->decodedFrame->linesize,
						0,
						player->videoCodecCtx->height,
						player->frameRGBA->data,
						player->frameRGBA->linesize
				);
				// store decoded and scaled frame to queue
				uint8_t *buffer = new uint8_t[player->avpictureSize];
				QueueData *queueData = new QueueData(buffer, player->videoCodecCtx->width * player->videoCodecCtx->height * 4);
				memcpy(queueData->buffer, player->buffer, player->videoCodecCtx->width * player->videoCodecCtx->height * 4);
				player->videoQueue->push(queueData);
			}
		}
		else if(packet.stream_index == player->audioStream && !isMute(pEnv, pObj)) {
			int len = avcodec_decode_audio4(player->audioCodecCtx, player->decodedFrame, &gotFrame, &packet);
			if(len < 0) {
				LOGE(pEnv, pObj, "error decoding audio frame");
			}
			//LOGD(pEnv, pObj, "frame channels: %d", player->decodedFrame->channels);
			packet.data += len;
			packet.size -= len;
			if(gotFrame) {
				if(!(convertedData = (uint8_t**) calloc(player->audioCodecCtx->channels,	sizeof(*convertedData)))) {
					LOGE(pEnv, pObj, "cannot allocate converted input sample pointers");
					return;
				}
				if( av_samples_alloc(convertedData, NULL, player->audioCodecCtx->channels, player->decodedFrame->nb_samples, AV_SAMPLE_FMT_S16, 0) < 0 ) {
					LOGE(pEnv, pObj, "cannot allocate converted input samples");
					return;
				}
				else{
					int outSamples = swr_convert(player->swrCtx, convertedData, player->decodedFrame->nb_samples, (uint8_t const **) player->decodedFrame->data, player->decodedFrame->nb_samples);

					if(outSamples < 0) {
						LOGE(pEnv, pObj, "audio convert error");
					}
					else {
						int bufferSize = av_samples_get_buffer_size(NULL, player->decodedFrame->channels, outSamples, AV_SAMPLE_FMT_S16,0);
						uint8_t *buffer = new uint8_t[bufferSize];
						QueueData *queueData = new QueueData(buffer, bufferSize);
						memcpy(queueData->buffer, convertedData[0], bufferSize);
						player->audioQueue->push(queueData);
						av_free(convertedData[0]);
					}
				}
			}
		}
		av_free_packet(&packet);
		// slow down
		if( player->videoQueue->size() > 30 * player->frameRate ) {
			LOGD(pEnv, pObj, "decode too fast! slowing down.");
			sleep(10);
		}

	}
	LOGD(pEnv, pObj, "naDecode done");
}

JNIEXPORT jbyteArray JNICALL Java_com_misgood_newlplayer_Player_naGetAudioData
(JNIEnv *pEnv, jobject pObj) {
	PlayerData *player = getPlayerData(pEnv, pObj);
	if(player->audioQueue->empty()) {
		LOGD(pEnv, pObj, "audio queue is empty");
		return NULL;
	}
	else {
		int bufferSize = player->audioQueue->front()->size;
		jbyteArray buffer = pEnv->NewByteArray(bufferSize);
		pEnv->SetByteArrayRegion(buffer, 0, bufferSize, (const signed char *) player->audioQueue->front()->buffer);
		// free buffer
		free(player->audioQueue->front()->buffer);
		player->audioQueue->pop();
		// write to AudioTrack
		return buffer;
	}
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naDisplay
(JNIEnv * pEnv, jobject pObj) {
	PlayerData *player = getPlayerData(pEnv, pObj);
	ANativeWindow_Buffer	 windowBuffer;
	uint8_t *buffer;

	if( player->videoQueue->empty() ) {
		LOGD(pEnv, pObj, "video queue is empty");
		return ERROR_QUEUE_IS_EMPTY;
	}
	else {
		buffer = player->videoQueue->front()->buffer;
		if (ANativeWindow_lock(player->window, &windowBuffer, NULL) < 0) {
			LOGE(pEnv, pObj, "cannot lock window");
			return ERROR_CANT_LOCK_WINDOW;
		}
		else {
			memcpy(windowBuffer.bits, buffer, player->videoCodecCtx->width * player->videoCodecCtx->height * 4);
		}
		ANativeWindow_unlockAndPost(player->window);
		free(buffer);
		player->videoQueue->pop();
		return 0;
	}
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naStopDecode
(JNIEnv * pEnv, jobject pObj) {
	PlayerData *player = getPlayerData(pEnv, pObj);
	LOGD(pEnv, pObj, "wait for stop decode");
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naRelease
(JNIEnv * pEnv, jobject pObj) {
	PlayerData *player = getPlayerData(pEnv, pObj);
	LOGD(pEnv, pObj, "naRelease start");
	// Release window
	ANativeWindow_release(player->window);
	// Free scaled image buffer
	av_free(player->buffer);
	// Free the RGB image
	av_free(player->frameRGBA);
	// Free the YUV frame
	av_free(player->decodedFrame);
	// Close the codec
	avcodec_close(player->videoCodecCtx);
	// Close the video file
	avformat_close_input(&player->formatCtx);
	// Free SWRContext
	swr_free(&player->swrCtx);
	// Free frame queue
	LOGD(pEnv, pObj, "naRelease done");
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naTest
(JNIEnv *pEnv, jobject pObj) {
	LOGI(pEnv, pObj, "native is activated");
}


