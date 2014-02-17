#include <stdio.h>
#include <pthread.h>
#include <jni.h>
#include <unistd.h>

#include <map>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "player.h"
#include "error.h"
#include "PlayerData.h"

#define LOG_TAG "Native"

#define LOGD(...) __android_log_print(3, LOG_TAG, __VA_ARGS__);
#define LOGI(...) __android_log_print(4, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(6, LOG_TAG, __VA_ARGS__);

std::map<jint, PlayerData*> playerMap;
JavaVM *jvm;

jint getHashCode(JNIEnv *env, jobject obj) {
	jclass cls = env->GetObjectClass(obj);
	jmethodID mid = env->GetMethodID(cls, "hashCode", "()I");
	if( mid == 0 ) {
		LOGE("cannot get hash code");
		return 0;
	}
	return env->CallIntMethod(obj, mid);
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naInit
(JNIEnv *pEnv, jobject pObj) {
	PlayerData *playerData;
	jint hashCode = getHashCode(pEnv, pObj);
	LOGD("naInit start");

	if(jvm == NULL) {
		pEnv->GetJavaVM(&jvm);
	}

	if( playerMap.find(hashCode) != playerMap.end() ) {
		LOGE("this player object is already initialized");
		return ERROR_ALREADY_INITIALIZED;
	}
	else {
		playerData = new PlayerData(pEnv, pObj);
		playerMap.insert(std::pair<jint, PlayerData*>(hashCode, playerData));
	}

	// Register all formats and codecs
	av_register_all();
	// if is network
	avformat_network_init();
	// Open video file

	LOGD("naInit done");
	return 0;
}

JNIEXPORT jintArray JNICALL Java_com_misgood_newlplayer_Player_naGetVideoRes
(JNIEnv *pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	PlayerData *playerData = playerMap.find(hashCode)->second;
	jintArray lRes;
	if (NULL == playerData->videoCodecCtx) {
		return NULL;
	}
	lRes = pEnv->NewIntArray(2);
	if (lRes == NULL) {
		LOGE("cannot allocate memory for video size");
		return NULL;
	}
	jint lVideoRes[2];
	lVideoRes[0] = playerData->videoCodecCtx->width;
	lVideoRes[1] = playerData->videoCodecCtx->height;
	pEnv->SetIntArrayRegion(lRes, 0, 2, lVideoRes);
	return lRes;
}

JNIEXPORT jdouble JNICALL Java_com_misgood_newlplayer_Player_naGetFps
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	return playerMap.find(hashCode)->second->frameRate;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naGetSampleRate
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	return playerMap.find(hashCode)->second->audioCodecCtx->sample_rate;
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naGetChannels
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	return playerMap.find(hashCode)->second->audioCodecCtx->channels;
}

void setSurface(PlayerData *pd, jobject pSurface) {
	if (pSurface) {
		pd->window = ANativeWindow_fromSurface(pd->env, pSurface);
	}
	else if(pd->window) {
		ANativeWindow_release(pd->window);
	}
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naSetup
(JNIEnv * pEnv, jobject pObj, jstring pFileName, jobject pSurface) {
	jint hashCode				= getHashCode(pEnv, pObj);
	PlayerData *playerData	= playerMap.find(hashCode)->second;
	AVCodec *pVideoCodec	= NULL;
	AVCodec *pAudioCodec	= NULL;
	AVDictionary *opts		= 0;

	LOGD("naSetup start");

	playerData->videoFileName = (char *) pEnv->GetStringUTFChars(pFileName, NULL);
	LOGI("video file name is %s", playerData->videoFileName);
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	if( avformat_open_input(&(playerData->formatCtx), playerData->videoFileName, NULL, &opts) != 0){
		LOGE("cannot open video.");
		return ERROR_CANT_OPEN_INPUT; // Couldn't open file
	}
	// Retrieve stream information
	if(avformat_find_stream_info(playerData->formatCtx, NULL)<0) {
		LOGE("cannot find stream information.");
		return ERROR_NO_STREAM_INFO_FOUND; // Couldn't find stream information
	}
	// Dump information about file onto standard error
	av_dump_format((playerData->formatCtx), 0, playerData->videoFileName, 0);
	av_dict_free(&opts);
	// Find best video stream
	playerData->videoStream = av_find_best_stream(playerData->formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if(playerData->videoStream == AVERROR_STREAM_NOT_FOUND) {
		LOGE("video stream not found.");
	}
	else if(playerData->videoStream == AVERROR_DECODER_NOT_FOUND) {
		LOGE("video stream is found, but no available decoder");
		return ERROR_NO_AVAILABLE_DECODER;
	}
	// Find best audio stream
	playerData->audioStream = av_find_best_stream(playerData->formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if(playerData->audioStream == AVERROR_STREAM_NOT_FOUND) {
		LOGI("audio stream not found.");
	}
	else if(playerData->audioStream == AVERROR_DECODER_NOT_FOUND) {
		LOGE("audio stream is found, but no available decoder");
		return ERROR_NO_AVAILABLE_DECODER;
	}
	if( playerData->audioStream < 0 && playerData->videoStream < 0 ) {
		LOGE("cannot find any media stream");
		return ERROR_STREAM_NOT_FOUND;
	}
	if( playerData->videoStream >= 0 ) {
		playerData->frameRate = av_q2d(playerData->formatCtx->streams[playerData->videoStream]->r_frame_rate);
		LOGI("frame rate: %f", playerData->frameRate);
		// Get a pointer to the codec context for the video stream
		playerData->videoCodecCtx = playerData->formatCtx->streams[playerData->videoStream]->codec;
		pVideoCodec = avcodec_find_decoder(playerData->videoCodecCtx->codec_id);
		if(pVideoCodec == NULL) {
			LOGE("unsupported codec");
			return -1; // Codec not found
		}
		// Open codec
		if(avcodec_open2(playerData->videoCodecCtx, pVideoCodec, NULL) < 0) {
			// Could not open codec
			LOGE("cannot open codec '%s'.", playerData->videoCodecCtx->codec_name);
			return ERROR_CANT_OPEN_DECODER;
		}
	}
	if( playerData->audioStream >= 0 ) {
		playerData->audioCodecCtx = playerData->formatCtx->streams[playerData->audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(playerData->audioCodecCtx->codec_id);
		if(pAudioCodec == NULL) {
			LOGE("unsupported codec");
			return -1; // Codec not found
		}
		if(avcodec_open2(playerData->audioCodecCtx, pAudioCodec, NULL) < 0) {
			LOGE("can not open codec %s.", playerData->audioCodecCtx->codec_name);
			return ERROR_CANT_OPEN_DECODER;
		}
	}

	playerData->decodedFrame = av_frame_alloc();
	if(playerData->decodedFrame == NULL) {
		LOGE("allocation fail.");
		return ERROR_FRAME_ALLOC_FAIL;
	}
	playerData->frameRGBA = av_frame_alloc();
	if(playerData->frameRGBA == NULL) {
		LOGE("allocation fail.");
		return ERROR_FRAME_ALLOC_FAIL;
	}

	// set surface
	setSurface(playerData, pSurface);

	// set format and size of window buffer
	ANativeWindow_setBuffersGeometry(playerData->window, playerData->videoCodecCtx->width, playerData->videoCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
	//get the scaling context
	if( !(playerData->swsCtx = sws_getCachedContext (
			NULL,
			playerData->videoCodecCtx->width,
			playerData->videoCodecCtx->height,
			playerData->videoCodecCtx->pix_fmt,
			playerData->videoCodecCtx->width,
			playerData->videoCodecCtx->height,
			AV_PIX_FMT_RGBA,
			SWS_X,
			NULL,
			NULL,
			NULL
	))) {
		LOGE("cannot get sws context");
		return ERROR_CANT_GET_SWS_CONTEXT;
	}
	playerData->avpictureSize = avpicture_get_size(AV_PIX_FMT_RGBA, playerData->videoCodecCtx->width, playerData->videoCodecCtx->height);
	playerData->buffer = (uint8_t*) av_malloc( playerData->avpictureSize * sizeof(uint8_t) );
	// Assign appropriate parts of bitmap to image planes in pFrameRGBA
	// Note that pFrameRGBA is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)playerData->frameRGBA, playerData->buffer, AV_PIX_FMT_RGBA, playerData->videoCodecCtx->width, playerData->videoCodecCtx->height);
	LOGD("naSetup done");
	return 0;
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naDecode
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode				= getHashCode(pEnv, pObj);
	PlayerData *playerData	= playerMap.find(hashCode)->second;
	AVPacket	 packet;
	int frameCount			= 0;
	int frameFinished;

	LOGD("naDecode start");
	playerData->stop = 0;
	packet.data = NULL;
	packet.size = 0;
	while(av_read_frame(playerData->formatCtx, &packet)>=0 && !playerData->stop) {
		// Is this a packet from the video stream?
		if(packet.stream_index == playerData->videoStream) {
			// Decode video frame
			if(avcodec_decode_video2(playerData->videoCodecCtx, playerData->decodedFrame, &frameFinished, &packet) < 0) {
				LOGE("error decoding video frame");
			}
			// Did we get a video frame?
			if(frameFinished) {
				// Convert the image from its native format to RGBA
				sws_scale(
						playerData->swsCtx,
						(uint8_t const * const *)playerData->decodedFrame->data,
						playerData->decodedFrame->linesize,
						0,
						playerData->videoCodecCtx->height,
						playerData->frameRGBA->data,
						playerData->frameRGBA->linesize
				);
				// store decoded and scaled frame to queue
				uint8_t *tmp = (uint8_t *) malloc(playerData->avpictureSize * sizeof(uint8_t));
				memcpy(tmp, playerData->buffer, playerData->videoCodecCtx->width * playerData->videoCodecCtx->height * 4);
				pthread_mutex_lock(&playerData->videoQueueLock);
				playerData->videoQueue.push(tmp);
				pthread_mutex_unlock(&playerData->videoQueueLock);

				// count number of frames
				++frameCount;
			}
		}
		else if(packet.stream_index == playerData->audioStream) {
			if(avcodec_decode_audio4(playerData->audioCodecCtx, playerData->decodedFrame, &frameFinished, &packet) < 0) {
				LOGE("error decoding audio frame");
			}
			if(frameFinished) {

			}

		}
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		// slow down
		if( playerData->videoQueue.size() > 30 * playerData->frameRate ) {
			LOGD("decode too fast! slowing down.");
			sleep(10);
		}
	}
	LOGD("total No. of frames decoded %d", frameCount);
}

JNIEXPORT jint JNICALL Java_com_misgood_newlplayer_Player_naDisplay
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	PlayerData *playerData = playerMap.find(hashCode)->second;
	ANativeWindow_Buffer	 windowBuffer;
	uint8_t *buffer;

	pthread_mutex_lock(&playerData->videoQueueLock);
	if( playerData->videoQueue.empty() ) {
		LOGD("queue is empty");
		pthread_mutex_unlock(&playerData->videoQueueLock);
		return ERROR_QUEUE_IS_EMPTY;
	}
	else {
		buffer = playerData->videoQueue.front();
		if (ANativeWindow_lock(playerData->window, &windowBuffer, NULL) < 0) {
			LOGE("cannot lock window");
			return ERROR_CANT_LOCK_WINDOW;
		}
		else {
			memcpy(windowBuffer.bits, buffer, playerData->videoCodecCtx->width * playerData->videoCodecCtx->height * 4);
		}
		ANativeWindow_unlockAndPost(playerData->window);
		free(buffer);
		playerData->videoQueue.pop();
		pthread_mutex_unlock(&playerData->videoQueueLock);
		return 0;
	}
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naStopDecode
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	PlayerData *playerData = playerMap.find(hashCode)->second;
	LOGD("wait for stop decode");
	playerData->stop = 1;
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naRelease
(JNIEnv * pEnv, jobject pObj) {
	jint hashCode = getHashCode(pEnv, pObj);
	PlayerData *playerData = playerMap.find(hashCode)->second;

	LOGD("naStop start");

	setSurface(playerData, NULL);
	// Free scaled image buffer
	av_free(playerData->buffer);
	// Free the RGB image
	av_free(playerData->frameRGBA);
	// Free the YUV frame
	av_free(playerData->decodedFrame);
	// Close the codec
	avcodec_close(playerData->videoCodecCtx);
	// Close the video file
	avformat_close_input(&playerData->formatCtx);
	// Free frame queue
	std::queue<uint8_t*> empty;
	std::swap( playerData->videoQueue, empty );

	LOGD("naStop done");
}

JNIEXPORT void JNICALL Java_com_misgood_newlplayer_Player_naTest
(JNIEnv *, jobject) {
	LOGI("native is activated");
}

