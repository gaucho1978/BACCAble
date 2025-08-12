/*
 * processingMessage0x00000226.h
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#ifndef INC_PROCESSINGSTANDARDMESSAGE_H_
	#define INC_PROCESSINGSTANDARDMESSAGE_H_

	#include "globalVariables.h"
	#include "processingMessage0x000000FC.h"
	#include "processingMessage0x000001EF.h"
	#include "processingMessage0x00000226.h"
	#include "processingMessage0x000002ED.h"
	#include "processingMessage0x000002FA.h"
	#include "processingMessage0x00000384.h"

	#if defined(C2baccable)
		#include "functions_C2baccable.h"
	#endif
	#if defined(BHbaccable)
		#include "functions_BHbaccable.h"
	#endif
    
	void processingStandardMessage();

#endif
