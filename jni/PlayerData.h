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
#include <libswresample/swresample.h>
}

template<class T>
class BlockingQueue{
public:
	BlockingQueue();
	~BlockingQueue();
	void push(T);
	void pop();
	bool empty();
	int size();
	const T front();

private:
	pthread_mutex_t lock;
	std::queue<T> queue;
};

class QueueData {
public:
	QueueData(uint8_t *, int);
	~QueueData();
	uint8_t	*buffer;
	int		size;
};

#ifndef PLAYERDATA_H_
#define PLAYERDATA_H_

class PlayerData {
public:
	PlayerData(JNIEnv *, jobject);
	~PlayerData();

	JNIEnv				*env = NULL;
	jobject				thiz;

	char				*videoFileName = NULL;
	int					avpictureSize = -1;
	int					videoStream = -1;
	int					audioStream = -1;
	double				frameRate = -1;

	ANativeWindow		*window = NULL;

	AVFormatContext 	*formatCtx = NULL;
	AVCodecContext		*audioCodecCtx = NULL;
	AVCodecContext		*videoCodecCtx = NULL;
	AVFrame				*decodedFrame = NULL;
	AVFrame				*frameRGBA = NULL;
	uint8_t				*buffer = NULL;
	SwsContext			*swsCtx = NULL;
	SwrContext			*swrCtx = NULL;

	BlockingQueue<QueueData>	videoQueue;
	BlockingQueue<QueueData>	audioQueue;

	bool				stop = false;
};

#endif /* PLAYERDATA_H_ */
