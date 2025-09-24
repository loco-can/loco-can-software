#pragma once

// #include "../../../config.h"
#include "../../core/can/can_com.h"


extern CAN_COM can;


class HANDSHAKE {

	public:

		void begin(void);
		void update(CAN_MESSAGE message);

	// private:
	// 	void error(bool);
	// 	bool error(void);

	// 	void mains(bool);
	// 	bool mains(void);

	// 	void paired(bool);
	// 	bool paired(void);

	// 	void ready(bool);
	// 	bool ready(void);

	// 	void direction(bool);
	// 	bool direction(void);
};