/*
 * QueueData.cpp
 *
 *  Created on: 2014/2/20
 *      Author: misgood
 */

#include <QueueData.h>

QueueData::QueueData(uint8_t *p, int s) {
	this->buffer = p;
	this->size = s;
}

QueueData::~QueueData() {
	free(this->buffer);
}
