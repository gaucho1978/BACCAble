#ifndef __MAIN_H
#define __MAIN_H

	#include "globalVariables.h"
	#include "functions_Common.h"

	#include "processingExtendedMessage.h"
	#include "processingStandardMessage.h"


	#if defined(C1baccable)
		#include "functions_C1baccable.h"
	#endif



	#if defined(BHbaccable)
		#include "functions_BHbaccable.h"
	#endif



#endif /* __MAIN_H */
