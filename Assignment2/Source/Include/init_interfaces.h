/*
 * init_interfaces.h
 *
 *  Created on: Apr 1, 2018
 *      Author: Ryan
 */

#ifndef INIT_INTERFACES_H_
#define INIT_INTERFACES_H_

typedef enum {
	STATIONARY, LAUNCH, RETURN
} system_mode_t;

typedef enum {
	OFF, ON
} warning_state;

void setup_interfaces();


#endif /* INIT_INTERFACES_H_ */
