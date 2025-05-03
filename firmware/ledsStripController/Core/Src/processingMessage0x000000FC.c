/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingMessage0x000000FC.h"

void processingMessage0x000000FC(){

	//message to dashboard containing rpm speed and not only
	#if (defined(C1baccable) || defined(C2baccable))
		if(rx_msg_header.DLC>=2){
			currentRpmSpeed=(rx_msg_data[0] *256 + (rx_msg_data[1] & ~0x3) )/4; //extract rpm speed
			//onboardLed_blue_on();
		}
	#endif
	#if defined(C2baccable)
		if(currentRpmSpeed< 400 ){
			if(ESCandTCinversion!=0) ESCandTCinversion=0;
			if(DynoModeEnabled!=0) DynoModeEnabled=0;
			if(front_brake_forced!=0) front_brake_forced=255;
		}
	#endif
	#if defined(C1baccable)
		if(currentRpmSpeed<400){
			startAndStopEnabled=1; //if motor off, re-enable start&stop logic
			requestToDisableStartAndStop=0; //re set request trigger

			if(_4wd_disabled>0){
				//disable 4dw function
				_4wd_disabled=0; //disable 4dw function
				dashboard_main_menu_array[main_dashboardPageIndex][4]=' '; //enabled
				dashboard_main_menu_array[main_dashboardPageIndex][5]='E';
				dashboard_main_menu_array[main_dashboardPageIndex][6]='n';
				commandsMenuEnabled=1;//enable menu commands
			}

			if(baccableDashboardMenuVisible==1){
				if(shutdownDashboardMenuRequestTime==0) shutdownDashboardMenuRequestTime=currentTime; //save time. we will shut it off after one minute
			}
		}else{
			shutdownDashboardMenuRequestTime=0;
			if(baccabledashboardMenuWasVisible==1){
				baccableDashboardMenuVisible=1; //show menu
				baccabledashboardMenuWasVisible=0; //avoid to return here
			}
		}
	#endif

	//engine speed fail is on byte1 bit 1.
	//engine StopStart Status is on byte 1 bit 0 and byte 2 bit 7
	//engine Status is on byte 2 bit 6 and bit 5.
	//gas pedal position is on byte 2 from bit 4 to 0 and byte 3 from bit 7 to 5.
	//gas pedal position fail is on byte3 bit 4.
	//.....
	//alternator fail is on byte 3, bit1.
	//stopStart status is on byte3 bit 0 and byte4 bit7.
	//CC brake intervention request is on byte 4, bit5
	//bank deactivation status is on byte5, bit 7 and 6
	//CC brake intervention is on byte 5 from bit 5 to 0 and byte 6 from bit 7 to 4.

}
