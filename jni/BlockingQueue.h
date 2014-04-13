/*
 * BlockingQueue.h
 *  Fixed Size Queue with mutex
 *
 *  Created on: 2014/2/20
 *      Author: misgood
 */

#include <pthread.h>
#include <queue>
#include <limits>

#ifndef BLOCKINGQUEUE_H_
#define BLOCKINGQUEUE_H_

#define MAX_INT std::numeric_limits<int>::max()

template<class T> class BlockingQueue{
public:
	BlockingQueue(int size) {
		pthread_mutex_init(&lock, NULL);
		maxSize = size;
	}

	BlockingQueue() {
		pthread_mutex_init(&lock, NULL);
		maxSize = MAX_INT;
	}

	~BlockingQueue() {
		pthread_mutex_destroy(&lock);
		std::queue<T> empty;
		std::swap( queue, empty );
	}

	void push(T val) {
		pthread_mutex_lock(&lock);
		if(queue.size() == maxSize) {
			queue.pop();
		}
		queue.push(val);
		pthread_mutex_unlock(&lock);
	}

	void pop() {
		pthread_mutex_lock(&lock);
		queue.pop();
		pthread_mutex_unlock(&lock);
	}

	bool empty() {
		bool ret;
		pthread_mutex_lock(&lock);
		ret = queue.empty();
		pthread_mutex_unlock(&lock);
		return ret;
	}

	int size() {
		int ret;
		pthread_mutex_lock(&lock);
		ret = queue.size();
		pthread_mutex_unlock(&lock);
		return ret;
	}

	const T front() {
		T ret;
		pthread_mutex_lock(&lock);
		ret = queue.front();
		pthread_mutex_unlock(&lock);
		return ret;
	}

private:
	pthread_mutex_t lock;
	std::queue<T> queue;
	int maxSize;
};

#endif /* BLOCKINGQUEUE_H_ */
