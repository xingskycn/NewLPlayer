/*
 * PlayerData.cpp
 *
 *  Created on: 2014/2/11
 *      Author: misgood
 */

#include <PlayerData.h>

PlayerData::PlayerData(JNIEnv *pEnv, jobject pObj) {
	this->audioQueue = new BlockingQueue<QueueData*>();
	this->videoQueue = new BlockingQueue<QueueData*>();
	this->env = pEnv;
	this->thiz = pObj;
}

PlayerData::~PlayerData() {
	delete this->audioQueue;
	delete this->videoQueue;
}
