/*
 * QueueData.h
 *
 *  Created on: 2014/2/20
 *      Author: misgood
 */

#include <unistd.h>

#ifndef QUEUEDATA_H_
#define QUEUEDATA_H_

class QueueData {
public:
	QueueData(uint8_t *, int);
	~QueueData();
	uint8_t	*buffer;
	int		size;
};

#endif /* QUEUEDATA_H_ */
