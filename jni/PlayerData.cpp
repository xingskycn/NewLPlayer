/*
 * PlayerData.cpp
 *
 *  Created on: 2014/2/11
 *      Author: misgood
 */

#include <PlayerData.h>

PlayerData::PlayerData(JNIEnv *pEnv, jobject pObj, jboolean isBuffer) {
	if(isBuffer) {
		this->audioQueue = new BlockingQueue<QueueData*>();
		this->videoQueue = new BlockingQueue<QueueData*>();
	}
	else{
		this->audioQueue = new BlockingQueue<QueueData*>(1);
		this->videoQueue = new BlockingQueue<QueueData*>(1);
	}
}

PlayerData::~PlayerData() {
	delete this->audioQueue;
	delete this->videoQueue;
}
