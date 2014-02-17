/*
 * PlayerData.cpp
 *
 *  Created on: 2014/2/11
 *      Author: misgood
 */

#include <PlayerData.h>

PlayerData::PlayerData(JNIEnv *pEnv, jobject pObj) {
	pthread_mutex_init(&videoQueueLock, NULL);
	pthread_mutex_init(&audioQueueLock, NULL);
	this->env = pEnv;
	this->thiz = pObj;
}

PlayerData::~PlayerData() {
	pthread_mutex_destroy(&videoQueueLock);
	pthread_mutex_destroy(&audioQueueLock);
}

