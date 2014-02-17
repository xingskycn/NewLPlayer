/*
 * PlayerData.h
 *
 *  Created on: 2014/2/11
 *      Author: misgood
 */

#include <stdio.h>
#include <pthread.h>
#include <queue>

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

#ifndef PLAYERDATA_H_
#define PLAYERDATA_H_

class PlayerData {
public:
	PlayerData(JNIEnv *, jobject);
	virtual ~PlayerData();

	JNIEnv				*env = NULL;
	jobject				thiz;

	char				*videoFileName = NULL;
	int					avpictureSize = -1;
	int					videoStream = -1;
	int					audioStream = -1;
	double				frameRate = -1;

	ANativeWindow		*window = NULL;

	AVFormatContext 		*formatCtx = NULL;
	AVCodecContext		*audioCodecCtx = NULL;
	AVCodecContext		*videoCodecCtx = NULL;
	AVFrame				*decodedFrame = NULL;
	AVFrame				*frameRGBA = NULL;
	uint8_t				*buffer = NULL;
	SwsContext			*swsCtx = NULL;

	std::queue<uint8_t *>	videoQueue;
	std::queue<uint8_t *>	audioQueue;

	pthread_mutex_t		videoQueueLock;
	pthread_mutex_t		audioQueueLock;

	bool				stop = 0;
};

#endif /* PLAYERDATA_H_ */