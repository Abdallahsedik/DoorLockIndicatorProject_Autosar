/*
 * DoorLock_Indication_SWC.c
 *
 *  Created on: Apr 9, 2026
 * Author: abdallah mohamed
 */

#include "D:\learn_in_depth\autosar_Door_Lock_Indicator_project\proj\01_F401rct6_Autosar_Project\AUTOSAR_Implementation\RTE_gen\Rte_DoorLock_indication_SWC.h"

#define LEDON  0
#define LEDOFF 1
//code for runnable
void DoorLock_indication_runnable()
{
	uint8 DoorState = 0;


	//Read DoorState from RTE sender receiver interface
	Rte_Read_DoorLock_indication_SWC_RP_SR_DoorState_DoorState(&DoorState);

	if (DoorState)
	{
		Rte_Call_RP_CS_Led_Switch_Led_Switch(LEDON);

	}else
	{
		Rte_Call_RP_CS_Led_Switch_Led_Switch(LEDOFF);
	}

}















