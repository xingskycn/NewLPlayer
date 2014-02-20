/*
 * PlayerData.cpp
 *
 *  Created on: 2014/2/11
 *      Author: misgood
 */

#include <PlayerData.h>

PlayerData::PlayerData(JNIEnv *pEnv, jobject pObj) {
	this->env = pEnv;
	this->thiz = pObj;
}

PlayerData::~PlayerData() {
}

QueueData::QueueData(uint8_t *p, int s) {
	this->buffer = p;
	this->size = s;
}

QueueData::~QueueData() {
	free(this->buffer);
}

template <class T>
BlockingQueue<T>::BlockingQueue() {
	pthread_mutex_init(&lock, NULL);
}

template <class T>
BlockingQueue<T>::~BlockingQueue() {
	pthread_mutex_destroy(&lock);
	std::queue<QueueData> empty;
	std::swap( queue, empty );
}

template <class T>
void BlockingQueue<T>::push(T val) {
	pthread_mutex_lock(&lock);
	queue.push(val);
	pthread_mutex_unlock(&lock);
}

template <class T>
void BlockingQueue<T>::pop() {
	pthread_mutex_lock(&lock);
	queue.pop();
	pthread_mutex_unlock(&lock);
}

template <class T>
bool BlockingQueue<T>::empty() {
	return queue.empty();
}

template <class T>
int BlockingQueue<T>::size() {
	return queue.size();
}

template <class T>
const T BlockingQueue<T>::front() {
	return queue.front();
}


