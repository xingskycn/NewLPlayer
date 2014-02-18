/*
 * error.h
 *
 *  Created on: 2014/2/11
 *      Author: misgood
 */

#ifndef ERROR_H_
#define ERROR_H_

// avformat_open_input
#define ERROR_CANT_OPEN_INPUT			1

// avformat_find_stream_info
#define ERROR_NO_STREAM_INFO_FOUND		2

// av_find_best_stream
#define ERROR_STREAM_NOT_FOUND			3
#define ERROR_NO_AVAILABLE_DECODER		4

// avcodec_find_decoder
#define ERROR_DECODER_NOT_FOUND			5

// avcodec_open2
#define ERROR_CANT_OPEN_DECODER			6

// av_frame_alloc
#define ERROR_FRAME_ALLOC_FAIL			7

// sws_getCachedContext
#define ERROR_CANT_GET_SWS_CONTEXT		8

// PlayerData
#define ERROR_ALREADY_INITIALIZED		9

// ANativeWindow_lock
#define ERROR_CANT_LOCK_WINDOW			10

// PlayerData->frameQueue
#define ERROR_QUEUE_IS_EMPTY			11

// swr_alloc
#define ERROR_CANT_ALLOCATE_SWR_CONTEXT	12

// swr_init
#define ERROR_INIT_SWR_CONTEXT_FAIL		13

#endif /* ERROR_H_ */
