/*
 * can.h
 *
 *  Created on: Nov 13, 2013
 *      Author: Nathan
 */

#ifndef CAN_H_
#define CAN_H_




struct CANmsg {
   char MBox;
   union CANMSGCTRL_REG   MSGCTRL;
   union CANMDL_REG       MDL;
   union CANMDH_REG       MDH;
};

struct TRS_REG {
	union CANTRS_REG	TRS;
};

void CANSetup();
char FillCAN(unsigned int Mbox);
void SendCAN(unsigned int Mbox);
void FillCANData();
void FillSendCAN(unsigned int Mbox);
void ClearMailBoxes();
void BUS_OFF();

//todo USER: DEFINE IDs for all output
//           DEFINE mailboxes for all output except heartbeat and command
#define COMMAND_ID 		0x1
#define HEARTBEAT_ID 	0x0

#define ADC_ID 			0x2
#define ADC_BOX 		2
#define ADC_TRS			TRS2

#define GP_BUTTON_ID 	0x3
#define GP_BUTTON_BOX 	3
#define GP_BUTTON_TRS	TRS3

#endif /* CAN_H_ */
