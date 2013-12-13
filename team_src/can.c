/*
 * can.c
 *
 *  Created on: Nov 12, 2013
 *      Author: Nathan
 */
#include "all.h"

void CANSetup()
{
	struct ECAN_REGS ECanaShadow;
	InitECanaGpio();
	InitECana();

	ECanaShadow.CANMIM.all = 0;
	ECanaShadow.CANMIL.all = 0;
	ECanaShadow.CANGIM.all = 0;
	ECanaShadow.CANGAM.bit.AMI = 0; //must be standard
	ECanaShadow.CANGIM.bit.I1EN = 1;  // enable I1EN
	ECanaShadow.CANMD.all = ECanaRegs.CANMD.all;
	ECanaShadow.CANME.all = ECanaRegs.CANME.all;

	//todo USER: Node specifc CAN setup

	// create mailbox for all Receive and transmit IDs
	// MBOX0 - MBOX31

	//Command RECEIVE
	ECanaShadow.CANMIM.bit.MIM0  = 1; 		//int enable
	ECanaShadow.CANMIL.bit.MIL0  = 1;  		// Int.-Level MB#1  -> I1EN
	ECanaMboxes.MBOX0.MSGID.bit.IDE = 0; 	//standard id
	ECanaMboxes.MBOX0.MSGID.bit.AME = 0;	// all bit must match
	ECanaMboxes.MBOX0.MSGID.bit.AAM = 0; 	// no RTR AUTO TRANSMIT
	ECanaRegs.CANMD.bit.MD0 = 1;			//receive
	ECanaRegs.CANME.bit.ME0 = 1;			//enable
	ECanaMboxes.MBOX0.MSGCTRL.bit.DLC = 8;
	ECanaMboxes.MBOX0.MSGID.bit.STDMSGID = COMMAND_ID;

	//Heart TRANSMIT
	ECanaMboxes.MBOX1.MSGID.bit.IDE = 0; 	//standard id
	ECanaMboxes.MBOX1.MSGID.bit.AME = 0; 	// all bit must match
	ECanaMboxes.MBOX1.MSGID.bit.AAM = 1; 	//RTR AUTO TRANSMIT
	ECanaRegs.CANMD.bit.MD1 = 0; 			//transmit
	ECanaRegs.CANME.bit.ME1 = 1;			//enable
	ECanaMboxes.MBOX1.MSGCTRL.bit.DLC = 8;
	ECanaMboxes.MBOX1.MSGID.bit.STDMSGID = HEARTBEAT_ID;

	//adc TRANSMIT
	ECanaMboxes.MBOX2.MSGID.bit.IDE = 0; 	//standard id
	ECanaMboxes.MBOX2.MSGID.bit.AME = 0; 	// all bit must match
	ECanaMboxes.MBOX2.MSGID.bit.AAM = 1; 	//RTR AUTO TRANSMIT
	ECanaRegs.CANMD.bit.MD2 = 0; 			//transmit
	ECanaRegs.CANME.bit.ME2 = 1;			//enable
	ECanaMboxes.MBOX2.MSGCTRL.bit.DLC = 8;
	ECanaMboxes.MBOX2.MSGID.bit.STDMSGID = ADC_ID;

	//gp_button TRANSMIT
	ECanaMboxes.MBOX3.MSGID.bit.IDE = 0; 	//standard id
	ECanaMboxes.MBOX3.MSGID.bit.AME = 0; 	// all bit must match
	ECanaMboxes.MBOX3.MSGID.bit.AAM = 1; 	//RTR AUTO TRANSMIT
	ECanaRegs.CANMD.bit.MD1 = 0; 			//transmit
	ECanaRegs.CANME.bit.ME1 = 1;			//enable
	ECanaMboxes.MBOX3.MSGCTRL.bit.DLC = 8;
	ECanaMboxes.MBOX3.MSGID.bit.STDMSGID = GP_BUTTON_ID;

	EALLOW;
	ECanaRegs.CANGAM.all = ECanaShadow.CANGAM.all;
	ECanaRegs.CANGIM.all = ECanaShadow.CANGIM.all;
	ECanaRegs.CANMIM.all = ECanaShadow.CANMIM.all;
	ECanaRegs.CANMIL.all = ECanaShadow.CANMIL.all;
    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.STM = 0;    // No self-test mode
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;
    EDIS;

    //ENABLE PIE INTERRUPTS
    IER |= M_INT9;
    PieCtrlRegs.PIEIER9.bit.INTx6= 1;
}



char FillCAN(unsigned int Mbox)
{
	//todo USER: setup for all transmit MBOXs
	switch (Mbox)								//choose mailbox
	{
	case HEARTBEAT_BOX:
		//todo Nathan define heartbeat
		ECanaMboxes.MBOX1.MDL.word.LOW_WORD = ops.Flags.all;
		return 1;
	case ADC_BOX:
		ECanaMboxes.MBOX1.MDL.all = data.adc;
		return 1;
	case GP_BUTTON_BOX:
		ECanaMboxes.MBOX1.MDL.all = data.gp_button;
		return 1;
	}
	return 0;
}

void FillSendCAN(unsigned Mbox)
{
	if (FillCAN(Mbox) == 1)
	{
		SendCAN(Mbox);
	}
}

void SendCAN(unsigned int Mbox)
{
	unsigned int mask = 1 << Mbox;
	ECanaRegs.CANTRS.all = mask;

	//todo Nathan: calibrate sendcan stopwatch
	stopwatch_struct* watch = StartStopWatch(SENDCAN_STOPWATCH);

	while(((ECanaRegs.CANTA.all & mask) != 1) && (isStopWatchComplete(watch) == 0)); //wait to send or hit stop watch

	ECanaRegs.CANTA.all = mask;						//clear flag
	if (isStopWatchComplete(watch) == 1)					//if stopwatch flag
	{
		ops.Flags.bit.can_error = 1;
	}
	else if (ops.Flags.bit.can_error == 1)		//if no stopwatch and flagged reset
	{
		ops.Flags.bit.can_error = 0;
	}

}


void SendCANBatch(struct TRS_REG *TRS)
{
	stopwatch_struct* watch = StartStopWatch(SENDCAN_STOPWATCH);

	ECanaRegs.CANTRS.all = TRS->TRS.all;
	while(((ECanaRegs.CANTA.all & TRS->TRS.all) != 1) && (isStopWatchComplete(watch) == 0));		//wait to send or stopwatch
	ECanaRegs.CANTA.all = TRS->TRS.all;

	if (isStopWatchComplete(watch) == 1)					//if stopwatch flag
	{
		ops.Flags.bit.can_error = 1;
	}
	else if (ops.Flags.bit.can_error == 1)		//if no stopwatch and flagged reset
	{
		ops.Flags.bit.can_error = 0;
	}
}

void FillCANData()
{
	//todo USER: use FillCAN to put data into correct mailboxes
	FillCAN(ADC_BOX);
	FillCAN(GP_BUTTON_BOX);
}

// INT9.6
__interrupt void ECAN1INTA_ISR(void)  // eCAN-A
{
	Uint32 ops_id;
	Uint32 dummy;
  	unsigned int mailbox_nr;
  	mailbox_nr = ECanaRegs.CANGIF1.bit.MIV1;
  	//todo USER: Setup ops command
  	if(mailbox_nr == COMMAND_BOX)
  	{
  		//todo Nathan: Define Command frame
  		//proposed:
  		//HIGH 4 BYTES = Uint32 ID
  		//LOW 4 BYTES = Uint32 change to
  		ops_id = ECanaMboxes.MBOX0.MDH.all;
  		dummy = ECanaMboxes.MBOX0.MDL.all;
		switch (ops_id)
		{
		case OPS_ID_STATE:
			memcpy(&ops.State,&dummy,sizeof ops.State);
			ops.Change.bit.State = 1;
			break;
		case OPS_ID_STOPWATCHERROR:
			memcpy(&ops.Flags.all,&dummy,sizeof ops.Flags.all);
			ops.Change.bit.Flags = 1;
			break;
		}
  	}
  	//todo USER: Setup other reads

  	//To receive more interrupts from this PIE group, acknowledge this interrupt
  	PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}
