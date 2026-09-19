// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#ifndef CAN_H
#define CAN_H

// #include "../../../config.h"


#ifdef MODULE_ARCH_ESP32
	/* ESP32 uses TWAI (ESP32_can); legacy SJA1000 is optional */
	#ifdef USE_LEGACY_ESP32_SJA1000
		#include "ESP32SJA1000.h"
	#endif
#else
	#include "MCP2515.h"
#endif

#endif
