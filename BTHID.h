/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.
 
 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").
 
 Contact information
 -------------------
 
 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 
 Modified 9 April 2013 by Shun Ito
 Web      :  http://ll0s0ll.wordpress.com/
*/


#ifndef _BTHID_h
#define _BTHID_h

#include "BTD.h"

/* StateMachine */
#define STATE_WAIT                                    0
#define STATE_SIMPLE_PAIRING_OPERATION                1
#define STATE_SDP_CONNECTION_CONFIGURATION_OPERATION  2
#define STATE_SDP_CONNECTION_ESTABLISHED              3
#define STATE_DISCONNECT_OPERATION                    4
#define STATE_SDP_OPERATION                          10

/* L2CAP event flags */
#define EV_FLAG_CONNECTION_REQUEST                  0x001
#define EV_FLAG_CONFIG_REQUEST                      0x004
#define EV_FLAG_CONFIG_RESPONSE_SUCCESS             0x010
#define EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_L2CAP    0x020
#define EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_UNKNOWN  0x040
#define EV_FLAG_SDP_SERVICE_ATTRIBUTE_REQUEST       0x080
#define EV_FLAG_DISCONNECT_REQUEST                  0x100

/* Used for SDP */
#define SDP_SERVICE_SEARCH_REQUEST_PDU       0x02
#define SDP_SERVICE_SEARCH_RESPONSE_PDU      0x03
#define SDP_SERVICE_ATTRIBUTE_REQUEST_PDU    0x04
#define SDP_SERVICE_ATTRIBUTE_RESPONSE_PDU   0x05

/* UUID */
#define L2CAP_UUID           0x0100
//#define PnPINFORMATION_UUID  0x1200

/* Destination Channel ID */
#define SDP_DCID       0x0050
#define HID_CTRL_DCID  0x0060
#define HID_INTR_DCID  0x0070

class BTHID : public BluetoothService {
public:
	/**
	 * Constructor for the BTHID class.
	 * @param  p   Pointer to BTD class instance.
	 */
	BTHID(BTD *p);
	
	/** @name BluetoothService implementation */
	/**
	 * Used to pass acldata to the services.
	 * @param ACLData Incoming acldata.
	 */
	virtual void ACLData(uint8_t* ACLData);
	/** Used to run part of the state maschine. */
	virtual void Run();
	/** Use this to reset the service. */
	virtual void Reset();
	/** Used this to disconnect any of the controllers. */
	virtual void disconnect();
	/**@}*/
	
	/** Indicate if the connection is established. */
	bool isConnected() {
		if	(HID_CTRL_Connected && HID_INTR_Connected)
			return true;
		else
			return false;
	}
	/* Send Keycodes to HID interrupt Channel.                                                  /
	/  About Keycode, See below document.                                                       /
	/  Universal Serial Bus HID Usage Tables - USB.org                                          /
	/  10 Keyboard/Keypad Page (0x07) p53-                                                      /
	/  http://www.usb.org/developers/devclass_docs/Hut1_11.pdf                                  /
	/  Keycode = UsageID. ex.keyboard [a] = 0x04.                                               /
	/  Keycode 0xE0-0xE7 are defined as modifier. To send LeftShiftKey[0xE1], modifier = 0xE1.  /
	/  6Keycodes send at same time                                                             */
	void HID_sendKeyCodes(uint8_t modifier, uint8_t keycode1, uint8_t keycode2, uint8_t keycode3, uint8_t keycode4, uint8_t keycode5, uint8_t keycode6);
	
	// Send Keycode 0x00 to HID interrupt Channel.(modifier and Key1-6)
	void HID_allKeyUp();
	
	// Convert ASCII to keycode. Return keycode.
	uint8_t HID_AsciitoKeycode(uint8_t ascii);

	// Send a character.
	void HID_SendCharacter(uint8_t ascii);
	
	//Maximum 32chars. Overflow ignored.
	void HID_sendString(char *str);
	
	//uint16 -32,768 to 32,767
	void HID_sendInteger(int16_t integer);
	void HID_sendFloat(float myfloat);
	
	
private:
	/* Mandatory members */
	BTD *pBtd;
	
	/* Set true when a channel is created */
	bool SDPConnected;
	bool HID_CTRL_Connected;
	bool HID_INTR_Connected;
	
	/* StateMachine */
	uint8_t  m_current_state;               // Store Current State
	uint16_t m_event_flag;                  // Store L2CAP EventFlag
	
	/* L2CAP Channels */
	uint8_t  l2capoutbuf[BULK_MAXPKTSIZE];  // General purpose buffer for l2cap out data
	uint8_t  m_transactionID_high;          // TransactionID High byte
	uint8_t  m_transactionID_low;           // TransactionID Low byte
	uint8_t  m_temp_scid[2];                // L2CAP source CID temporary
	uint8_t  m_temp_dcid[2];                // L2CAP destination CID temporary
	uint8_t  m_sdp_scid[2];                 // L2CAP source CID for SDP
	uint8_t  m_control_scid[2];             // L2CAP source CID for HID_Control
	uint8_t  m_interrupt_scid[2];           // L2CAP source CID for HID_Interrupt
	uint8_t  m_identifier;                  // Identifier for connection
	
	/* SDP Commands */
	void SDP_Command(uint8_t* data, uint8_t nbytes);
	void sdp_serviceNotSupported();
	void sdp_ServiceSearchResponse();
	void sdp_ServiceAttributeResponse();
	
	/* HID Commands */
	void HID_Command(uint8_t* data, uint8_t nbytes);
	void HID_Command_interrupt(uint8_t* data, uint8_t nbytes);
	void HID_Handshake(uint8_t result_code);
};

#endif
