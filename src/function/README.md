# FUNCTIONS

Functions are program parts for a certain functionality. A hardware module is
built with different functions from the pool.

A function must be stored in the src/function subdirectory as main.h/main.cpp.
It is included in the module h file. In addition the function must be initialized
in the LocoCANcore.h and started in the LocoCANcore.cpp.

All function must have two public methods:

	void begin(void);  // initialisation
	void update(void); // update the function in the main loop

