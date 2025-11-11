#include "handshake.h"


void HANDSHAKE::begin(uint16_t timeout) {

	_timeout.begin(timeout);

	_master_status.set(0);
	_slave_status.set(0);
}


/* ***************************************************/
/* calculate master status */
void HANDSHAKE::update_master(CAN_MESSAGE message) {
	update_master(message.data[0]);
}

void HANDSHAKE::update_master(FLAGS status) {
	_master_status = status;
}

void HANDSHAKE::update_master(uint8_t status) {

// TODO add calculation of status

/*
	? can be activated
		Y:	? master.main_switch is on
			Y:	? is active
				Y:	status == setup
					END
				N:	? nulled == true
					Y:	set->master nulled
						? drive == on
							N:	set>master standby
								END
							Y:	slave.moving == true
								Y:	set>master moving
									END
								N:	set>master standby
									END
					N:	set>master not nulled
						END
			N:	|| current vehicle in list ||
				set>master off
				set>master active false
		N:	|| reset vehicle list ||
			set>master locked
			set>master active false
			END

	|| set status LED ||
 */

	








	_master_status.set(status);
}


/* ***************************************************/
/* calculate slave status */
void HANDSHAKE::update_slave(CAN_MESSAGE message) {
	update_master(message.data[0]);
}

void HANDSHAKE::update_slave(FLAGS status) {
	_master_status = status;
}

void HANDSHAKE::update_slave(uint8_t status) {


// TODO add calculation of status



	_slave_status.set(status);
}


/* ***************************************************/
/* get status */
FLAGS HANDSHAKE::master_status(void) {
	return _master_status;
}

FLAGS HANDSHAKE::slave_status(void) {
	return _slave_status;
}


/* ***************************************************/
/* send message */
void HANDSHAKE::send_master(CAN_MESSAGE message) {

}

void HANDSHAKE::send_slave(CAN_MESSAGE message) {

}