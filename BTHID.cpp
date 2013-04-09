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


#include "BTHID.h"
#define DEBUG  // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data

// asciiandkeycode[ascii] = keycode
// ex. 'a' -> asciiandkeycode[0x61] = 0x04
//  Keycode = UsageID. About Keycode, See below document.
// Universal Serial Bus HID Usage Tables - USB.org
// 10 Keyboard/Keypad Page (0x07) p53-
// http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
const uint8_t asciiandkeycode[128] PROGMEM = {
//    0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00,  // 0
	0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00,  // 1
	0x2C, 0x1E,	0x34, 0x20, 0x21, 0x22, 0x24, 0x34, 0x26, 0x27, 0x25, 0x2E, 0x36, 0x2D, 0x37, 0x38,  // 2
	0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38,  // 3
	0x1F, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,  // 4
	0x13, 0x14, 0x15, 0x16,	0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x2F, 0x31, 0x30, 0x23, 0x2D,  // 5
	0x35, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,  // 6
	0x13, 0x14, 0x15, 0x16,	0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x2F, 0x31, 0x30, 0x35, 0x2A   // 7
};

BTHID::BTHID(BTD *p):
pBtd(p) // pointer to USB class instance - mandatory
{
	if (pBtd)
		pBtd->registerServiceClass(this); // Register it as a Bluetooth service
	
	pBtd->btdName = "HOGEra";
	Reset();
}

void BTHID::Reset() {//virtual
	SDPConnected = false;
	HID_CTRL_Connected = false;
	HID_INTR_Connected = false;
	//Reset state
	m_current_state = STATE_WAIT;
}

void BTHID::disconnect() {//virtual
#ifdef DEBUG
	Notify(PSTR("\r\ndisconnect"));
#endif
//	connected = false;
	m_current_state = STATE_DISCONNECT_OPERATION;
}

void BTHID::ACLData(uint8_t* l2capinbuf) {//virtual
	if (((l2capinbuf[0] | (l2capinbuf[1] << 8)) == (pBtd->hci_handle | 0x2000))) { // acl_handle_ok
		switch ((l2capinbuf[6] | (l2capinbuf[7] << 8)))
		{
			///-------------- L2CAP Control -------------///
			case 0x0001: //l2cap_control - Channel ID for ACL-U
				///--- L2CAP_CMD_COMMAND_REJECT ---///
				if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG
					Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "));
					PrintHex<uint8_t>(l2capinbuf[13]);
					Notify(PSTR(" "));
					PrintHex<uint8_t>(l2capinbuf[12]);
					Notify(PSTR(" Data: "));
					PrintHex<uint8_t>(l2capinbuf[17]);
					Notify(PSTR(" "));
					PrintHex<uint8_t>(l2capinbuf[16]);
					Notify(PSTR(" "));
					PrintHex<uint8_t>(l2capinbuf[15]);
					Notify(PSTR(" "));
					PrintHex<uint8_t>(l2capinbuf[14]);
#endif
				///--- L2CAP_CMD_CONNECTION_REQUEST ---///
				} else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {
#ifdef EXTRADEBUG
					Notify(PSTR("\r\nL2CAP Connection Request - PSM: "));
					PrintHex<uint8_t>(l2capinbuf[13]);
					Notify(PSTR(" "));
					PrintHex<uint8_t>(l2capinbuf[12]);
					Notify(PSTR(" SCID: "));
					PrintHex<uint8_t>(l2capinbuf[15]);
					Notify(PSTR(" "));
					PrintHex<uint8_t>(l2capinbuf[14]);
					Notify(PSTR(" Identifier: "));
					PrintHex<uint8_t>(l2capinbuf[9]);
#endif
					//Reset state
					m_current_state = STATE_WAIT;
					//Check PSM
					if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_PSM) {
						//Set DestinationCID
						m_temp_dcid[0] = (uint8_t)(SDP_DCID & 0xFF); //0x0050
						m_temp_dcid[1] = (uint8_t)((SDP_DCID >> 8) & 0xFF);
						m_identifier = l2capinbuf[9];
						//Set SourceCID
						m_temp_scid[0] = l2capinbuf[14];
						m_temp_scid[1] = l2capinbuf[15];
						//Store SDP SourceCID
						m_sdp_scid[0] = l2capinbuf[14];
						m_sdp_scid[1] = l2capinbuf[15];
						//Chage Flag
						m_event_flag |= EV_FLAG_CONNECTION_REQUEST;
					} else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == HID_CTRL_PSM) {
						//DestinationCID
						m_temp_dcid[0] = (uint8_t)(HID_CTRL_DCID & 0xFF); //0x0060
						m_temp_dcid[1] = (uint8_t)((HID_CTRL_DCID >> 8) & 0xFF);
	 
						m_identifier = l2capinbuf[9];
						//SourceCID
						m_temp_scid[0] = l2capinbuf[14];
						m_temp_scid[1] = l2capinbuf[15];
						m_control_scid[0] = l2capinbuf[14];
						m_control_scid[1] = l2capinbuf[15];
						m_event_flag |= EV_FLAG_CONNECTION_REQUEST;
					} else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == HID_INTR_PSM) {
						//DestinationCID
						m_temp_dcid[0] = (uint8_t)(HID_INTR_DCID & 0xFF); // 0x0070;
						m_temp_dcid[1] = (uint8_t)((HID_INTR_DCID >> 8) & 0xFF);

						m_identifier = l2capinbuf[9];
						//SourceCID
						m_temp_scid[0] = l2capinbuf[14];
						m_temp_scid[1] = l2capinbuf[15];
						m_interrupt_scid[0] = l2capinbuf[14];
						m_interrupt_scid[1] = l2capinbuf[15];
						m_event_flag |= EV_FLAG_CONNECTION_REQUEST;
					}
				///--- L2CAP_CMD_CONFIG_RESPONSE ---///
				} else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
					if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { // Success
						if (l2capinbuf[12] == m_temp_dcid[0] && l2capinbuf[13] == m_temp_dcid[1]) {
							//Serial.print("\r\nSDP Configuration Complete");
							m_event_flag |= EV_FLAG_CONFIG_RESPONSE_SUCCESS;
						}
					}
				///--- L2CAP_CMD_CONFIG_REQUEST ---///
				} else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
					if (l2capinbuf[12] == m_temp_dcid[0] && l2capinbuf[13] == m_temp_dcid[1]) {
						//Serial.print("\r\nSDP Configuration Request");
#ifdef EXTRADEBUG
						Notify(PSTR("\r\nCONFIG_REQUEST Flag: "));
						PrintHex<uint8_t>(l2capinbuf[14]);
						Notify(PSTR(" "));
						PrintHex<uint8_t>(l2capinbuf[15]);
						Notify(PSTR(" Option: "));
						PrintHex<uint8_t>(l2capinbuf[16]);
						Notify(PSTR(" "));
						PrintHex<uint8_t>(l2capinbuf[17]);
						Notify(PSTR(" "));
						PrintHex<uint8_t>(l2capinbuf[18]);
						Notify(PSTR(" "));
						PrintHex<uint8_t>(l2capinbuf[19]);
#endif
						m_identifier = l2capinbuf[9];
						m_event_flag |= EV_FLAG_CONFIG_REQUEST;
					}
				///--- L2CAP_CMD_DISCONNECT_REQUEST ---///
				} else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
					if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_DCID) {
						Notify(PSTR("\r\nDisconnect Request: SDP Channel"));
						m_identifier = l2capinbuf[9];
						//Reset state
						m_current_state = STATE_DISCONNECT_OPERATION;
						m_event_flag |= EV_FLAG_DISCONNECT_REQUEST;
					} else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == HID_CTRL_DCID) {
						Notify(PSTR("\r\nDisconnect Request: HID_CTRL Channel"));
					} else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == HID_INTR_DCID) {
						Notify(PSTR("\r\nDisconnect Request: HID_INTR Channel"));
					} else {
						Notify(PSTR("\r\nDisconnect Request: UNKNOWN Channel"));
					}
				///--- L2CAP_CMD_DISCONNECT_RESPONSE ---///
				} else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE) {
					Notify(PSTR("\r\nL2CAP_CMD_DISCONNECT_RESPONSE:"));
					if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_DCID) {
						Notify(PSTR(" SDP Channel"));
						SDPConnected = false;
					} else if ((l2capinbuf[14] | (l2capinbuf[15] << 8)) == HID_CTRL_DCID) {
						Notify(PSTR(" HID_CTRL Channel"));
						HID_CTRL_Connected = false;
					} else if ((l2capinbuf[14] | (l2capinbuf[15] << 8)) == HID_INTR_DCID) {
						Notify(PSTR(" HID_INTR Channel"));
						HID_INTR_Connected = false;
					}
				} else {
					Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "));
					PrintHex<uint8_t>(l2capinbuf[8]);
				}
				break;

			///-------------- SDP Protocol --------------///
			case SDP_DCID:
#ifdef EXTRADEBUG			
				Notify(PSTR("\r\nSDP Protocol -"));
				Notify(PSTR(" PDUID:"));
				PrintHex<uint8_t>(l2capinbuf[8]);
				Notify(PSTR(" TransuctionID:"));
				PrintHex<uint8_t>(l2capinbuf[9]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[10]);
				Notify(PSTR(" ParameterL:"));
				PrintHex<uint8_t>(l2capinbuf[11]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[12]);
				Notify(PSTR(" - "));
				PrintHex<uint8_t>(l2capinbuf[13]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[14]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[15]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[16]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[17]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[18]);
				Notify(PSTR(" "));
				for (int i=0; i < l2capinbuf[12]-6; i++) {
					PrintHex<uint8_t>(l2capinbuf[19+i]);
					Notify(PSTR(" "));
				}
#endif
				///--- SDP_SERVICE_SEARCH_REQUEST_PDU ---///
				if(l2capinbuf[8] == SDP_SERVICE_SEARCH_REQUEST_PDU) {
//					Notify(PSTR("\r\nSDP_SERVICE_SEARCH_REQUEST_PDU"));
					///--- L2CAP_UUID ---///
					if ((l2capinbuf[16] << 8 | l2capinbuf[17]) == L2CAP_UUID) {
						m_transactionID_high = l2capinbuf[9];
						m_transactionID_low = l2capinbuf[10];
						m_event_flag |= EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_L2CAP;
					} else {
//						Notify(PSTR("\r\nUUID ServiceNotSupported"));
						m_transactionID_high = l2capinbuf[9];
						m_transactionID_low = l2capinbuf[10];
						m_event_flag |= EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_UNKNOWN;
					}
				///--- SDP_SERVICE_ATTRIBUTE_REQUEST_PDU ---///
				} else if(l2capinbuf[8] == SDP_SERVICE_ATTRIBUTE_REQUEST_PDU) {
					m_transactionID_high = l2capinbuf[9];
					m_transactionID_low = l2capinbuf[10];
					m_event_flag |= EV_FLAG_SDP_SERVICE_ATTRIBUTE_REQUEST;
				} else {
					Notify(PSTR("\r\nSDP Unknown Signaling Command: "));
					PrintHex<uint8_t>(l2capinbuf[8]);
				}
				break;

			///---------------- HID Ctrl ----------------///
			case HID_CTRL_DCID:
#ifdef EXTRADEBUG
				Notify(PSTR("\r\nHID Protocol CID:"));
				PrintHex<uint8_t>(l2capinbuf[6]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[7]);
				Notify(PSTR(" THdr"));
				for (int i=0; i < l2capinbuf[4]; i++) {
					PrintHex<uint8_t>(l2capinbuf[8+i]);
					Notify(PSTR(" "));
				}
#endif
				///--- SET_PROTOCOL request(Report) ---///
				if (l2capinbuf[8] == 0x71) {
					HID_Handshake((uint8_t)0x00); //SUCCESSFUL
				///--- HID_CONTROL request(SUSPEND) ---///
				} else if (l2capinbuf[8] == 0x13) {
#ifdef DEBUG
					Notify(PSTR("\r\nHID_CONTROL request - SUSPEND. Go to reduced power mode. zzz.."));
#endif
				}
				break;
			
			default:
				Notify(PSTR("\r\nUnsupported L2CAP Data - Channel ID: "));
				PrintHex<uint8_t>(l2capinbuf[7]);
				Notify(PSTR(" "));
				PrintHex<uint8_t>(l2capinbuf[6]);
				break;
		} //switch
	}
}

void BTHID::Run() //virtual
{
	switch (m_current_state)
	{
		///------ STATE_WAIT ------///
		case STATE_WAIT:
			if (m_event_flag & EV_FLAG_CONNECTION_REQUEST)	{
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] CONNECTION_REQUEST"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_CONNECTION_REQUEST;
				pBtd->l2cap_connection_response(pBtd->hci_handle,m_identifier, m_temp_dcid, m_temp_scid, PENDING);
				//Simple pairing Operation Start
				if (!pBtd->isSimplePairingCompleted()) {
					pBtd->StartSimplePairingOperation(pBtd->hci_handle);
					//Change state
					m_current_state = STATE_SIMPLE_PAIRING_OPERATION;
				} else {
					pBtd->l2cap_connection_response(pBtd->hci_handle,m_identifier, m_temp_dcid, m_temp_scid, SUCCESSFUL);
					m_identifier++;
					m_current_state = STATE_SDP_CONNECTION_CONFIGURATION_OPERATION;
				}
			}
			break;
		///------ STATE_SIMPLE_PAIRING_OPERATION ------///
		case STATE_SIMPLE_PAIRING_OPERATION:
			if (pBtd->isSimplePairingCompleted()) {
#ifdef DEBUG
				Notify(PSTR("\r\nSIMPLE_PAIRING_OPERATION COMPLETED"));
#endif
				pBtd->l2cap_connection_response(pBtd->hci_handle,m_identifier, m_temp_dcid, m_temp_scid, SUCCESSFUL);
				m_identifier++;
				//Change state
				m_current_state = STATE_SDP_CONNECTION_CONFIGURATION_OPERATION;
			}
			break;
		///------ STATE_SDP_CONNECTION_CONFIGURATION_OPERATION ------///
		case STATE_SDP_CONNECTION_CONFIGURATION_OPERATION:
			if (m_event_flag & EV_FLAG_CONFIG_REQUEST) {
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] CONFIG_REQUEST"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_CONFIG_REQUEST;
				pBtd->l2cap_config_response(pBtd->hci_handle,m_identifier, m_temp_scid);
				delay(1);
				pBtd->l2cap_config_request(pBtd->hci_handle,m_identifier, m_temp_scid);
			}
			
			if (m_event_flag & EV_FLAG_CONFIG_RESPONSE_SUCCESS) {
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] CONFIG_RESPONSE_SUCCESS"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_CONFIG_RESPONSE_SUCCESS;
				
				if ((m_temp_dcid[0] | (m_temp_dcid[1] << 8)) == SDP_DCID) {
					SDPConnected = true;
				} else if ((m_temp_dcid[0] | (m_temp_dcid[1] << 8)) == HID_CTRL_DCID) {
					HID_CTRL_Connected = true;
				} else if ((m_temp_dcid[0] | (m_temp_dcid[1] << 8)) == HID_INTR_DCID) {					
					HID_INTR_Connected = true;
				}
					
				//Change state
				m_current_state = STATE_SDP_OPERATION;
			}
			break;
		///------ STATE_DISCONNECT_OPERATION ------///
		case STATE_DISCONNECT_OPERATION:
			if(m_event_flag & EV_FLAG_DISCONNECT_REQUEST) {
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] DISCONNECT_REQUEST"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_DISCONNECT_REQUEST;
				SDPConnected = false;
				uint8_t disconnect_dcid[2];
				disconnect_dcid[0] = (uint8_t)(SDP_DCID & 0xFF);
				disconnect_dcid[1] = (uint8_t)((SDP_DCID >> 8) & 0xFF);
				pBtd->l2cap_disconnection_response(pBtd->hci_handle, m_identifier, disconnect_dcid, m_sdp_scid);
				m_current_state = STATE_WAIT;
				break;
			}
			
			// First the two L2CAP channels has to be disconencted and then the HCI connection
			if(SDPConnected) {
#ifdef DEBUG
				Notify(PSTR("\r\ndisconnect sdp"));
#endif
				uint8_t disconnect_dcid[2];
				disconnect_dcid[0] = (uint8_t)(SDP_DCID & 0xFF);
				disconnect_dcid[1] = (uint8_t)((SDP_DCID >> 8) & 0xFF);
				pBtd->l2cap_disconnection_request(pBtd->hci_handle, m_identifier, m_sdp_scid, disconnect_dcid);
				SDPConnected = false;
				delay(5);
			} else if(HID_CTRL_Connected) {
#ifdef DEBUG
				Notify(PSTR("\r\ndisconnect HID_CTRL"));
#endif
				uint8_t disconnect_dcid[2];
				disconnect_dcid[0] = (uint8_t)(HID_CTRL_DCID & 0xFF);
				disconnect_dcid[1] = (uint8_t)((HID_CTRL_DCID >> 8) & 0xFF);
				pBtd->l2cap_disconnection_request(pBtd->hci_handle, m_identifier, m_control_scid, disconnect_dcid);
				HID_CTRL_Connected = false;
				delay(5);
			} else if(HID_INTR_Connected) {
#ifdef DEBUG
				Notify(PSTR("\r\ndisconnect HID_INTR"));
#endif
				uint8_t disconnect_dcid[2];
				disconnect_dcid[0] = (uint8_t)(HID_INTR_DCID & 0xFF);
				disconnect_dcid[1] = (uint8_t)((HID_INTR_DCID >> 8) & 0xFF);
				pBtd->l2cap_disconnection_request(pBtd->hci_handle, m_identifier, m_interrupt_scid, disconnect_dcid);
				HID_INTR_Connected = false;
				delay(5);
			} else {
#ifdef DEBUG
				Notify(PSTR("\r\nhci_disconnect"));
#endif
				pBtd->hci_disconnect(pBtd->hci_handle);
				pBtd->hci_handle = -1;
				m_current_state = STATE_WAIT;
			}
			break;
		///------ STATE_SDP_OPERATION ------///
		case STATE_SDP_OPERATION:
			if (m_event_flag & EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_L2CAP) {
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] SDP_SERVICE_SEARCH_REQUEST_L2CAP"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_L2CAP;
				sdp_ServiceSearchResponse();
			}
			
			if (m_event_flag & EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_UNKNOWN) {
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] SDP_SERVICE_SEARCH_REQUEST_UNKNOWN"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_SDP_SERVICE_SEARCH_REQUEST_UNKNOWN;
				sdp_serviceNotSupported();
			}
			
			if (m_event_flag & EV_FLAG_SDP_SERVICE_ATTRIBUTE_REQUEST) {
#ifdef DEBUG
				Notify(PSTR("\r\n[EV_FLAG] SDP_SERVICE_ATTRIBUTE_REQUEST"));
#endif
				//Clear EventFlag
				m_event_flag &= ~EV_FLAG_SDP_SERVICE_ATTRIBUTE_REQUEST;
				sdp_ServiceAttributeResponse();
			}
			break;
	}//switch
}

/************************************************************/
/*                    SDP Commands                          */
/************************************************************/
void BTHID::SDP_Command(uint8_t* data, uint8_t nbytes) {
	pBtd->L2CAP_Command(pBtd->hci_handle,data,nbytes,m_sdp_scid[0],m_sdp_scid[1]);
}

void BTHID::sdp_serviceNotSupported() {
	l2capoutbuf[0] = SDP_SERVICE_SEARCH_RESPONSE_PDU;
	l2capoutbuf[1] = m_transactionID_high; //transactionIDHigh;
	l2capoutbuf[2] = m_transactionID_low; //transactionIDLow;
	l2capoutbuf[3] = 0x00; // Parameter Length
	l2capoutbuf[4] = 0x05;
	l2capoutbuf[5] = 0x00; // TotalServiceRecordCount
	l2capoutbuf[6] = 0x00;
	l2capoutbuf[7] = 0x00; // CurrentServiceRecordCount
	l2capoutbuf[8] = 0x00;
	l2capoutbuf[9] = 0x00; // No more data
	
	SDP_Command(l2capoutbuf,10);
}

void BTHID::sdp_ServiceSearchResponse() {
	l2capoutbuf[0] = SDP_SERVICE_SEARCH_RESPONSE_PDU;
	l2capoutbuf[1] = m_transactionID_high; //transactionIDHigh;
	l2capoutbuf[2] = m_transactionID_low; //transactionIDLow;
	l2capoutbuf[3] = 0x00; // Parameter Length
	l2capoutbuf[4] = 0x09;
	l2capoutbuf[5] = 0x00; // TotalServiceRecordCount
	l2capoutbuf[6] = 0x01; 
	
	/* Attribute ID/Value Sequence: */
	l2capoutbuf[7] = 0x00; // CurrentServiceRecordCount
	l2capoutbuf[8] = 0x01; 
	l2capoutbuf[9] = 0x00; // ServiceRecordHandleList - 0x00010000
	l2capoutbuf[10] = 0x01;
	l2capoutbuf[11] = 0x00;
	l2capoutbuf[12] = 0x00;
	l2capoutbuf[13] = 0x00; // No more data
	
	SDP_Command(l2capoutbuf,14);
}
void BTHID::sdp_ServiceAttributeResponse() {
	l2capoutbuf[0] = SDP_SERVICE_ATTRIBUTE_RESPONSE_PDU;
	l2capoutbuf[1] = m_transactionID_high; //transactionIDHigh;
	l2capoutbuf[2] = m_transactionID_low; //transactionIDLow;
	l2capoutbuf[3] = 0x00; // Parameter Length (AttributeListByteCount + 2 + 1) biged
	l2capoutbuf[4] = 0x00; // 9D {(tmp_size+1) - 5}
	l2capoutbuf[5] = 0x00; // AttributeListByteCount (All - (7 + 1))
	l2capoutbuf[6] = 0x00; // 9A {(tmp_size+1) - (7+1)}
	
	//Sequence
	l2capoutbuf[7] = 0x35; // total2
	l2capoutbuf[8] = 0x00; // 0x98 {(tmp_size+1) - (7+1+2)}
	//Service Class ID List // total8
	l2capoutbuf[9] = 0x09; // 0x0001 - ServiceClassIDList
	l2capoutbuf[10] = 0x00;
	l2capoutbuf[11] = 0x01;
	l2capoutbuf[12] = 0x35; // UUID - 0x1124(HumanInterfaceDeviceService)
	l2capoutbuf[13] = 0x03;
	l2capoutbuf[14] = 0x19;
	l2capoutbuf[15] = 0x11;
	l2capoutbuf[16] = 0x24;

	//ProtocolDescriptorList //tatal 18
	l2capoutbuf[17] = 0x09; // 0x0004 - ProtocolDescriptorList
	l2capoutbuf[18] = 0x00; //
	l2capoutbuf[19] = 0x04; //
	l2capoutbuf[20] = 0x35;
	l2capoutbuf[21] = 0x0D;
	l2capoutbuf[22] = 0x35;
	l2capoutbuf[23] = 0x06;
	l2capoutbuf[24] = 0x19; // UUID=0x0100 L2CAP
	l2capoutbuf[25] = 0x01; //
	l2capoutbuf[26] = 0x00; //
	l2capoutbuf[27] = 0x09; // PSM=0x0011 HID_Control
	l2capoutbuf[28] = 0x00; //
	l2capoutbuf[29] = 0x11; //
	l2capoutbuf[30] = 0x35;
	l2capoutbuf[31] = 0x03;
	l2capoutbuf[32] = 0x19; // UUID=0x0011 HIDP Human Interface Device Profile
	l2capoutbuf[33] = 0x00; //
	l2capoutbuf[34] = 0x11; //

	//AdditionalProtocolDescriptorLists //tatal 20
	l2capoutbuf[35] = 0x09; // 0x000D
	l2capoutbuf[36] = 0x00; //
	l2capoutbuf[37] = 0x0D; //
	l2capoutbuf[38] = 0x35;
	l2capoutbuf[39] = 0x0F; //0C
	l2capoutbuf[40] = 0x35;
	l2capoutbuf[41] = 0x08; //0A
	l2capoutbuf[42] = 0x35;
	l2capoutbuf[43] = 0x06; //03
	l2capoutbuf[44] = 0x19; // UUID=0x0100 L2CAP
	l2capoutbuf[45] = 0x01;
	l2capoutbuf[46] = 0x00;
	l2capoutbuf[47] = 0x09; // PSM=0x0013 HID_Interrrupt
	l2capoutbuf[48] = 0x00;
	l2capoutbuf[49] = 0x13;
	l2capoutbuf[50] = 0x35; // UUID=0x0011 HIDP
	l2capoutbuf[51] = 0x03;
	l2capoutbuf[52] = 0x19;
	l2capoutbuf[53] = 0x00;
	l2capoutbuf[54] = 0x11;
	
	//HIDDeviceSubclass (total 5)
	l2capoutbuf[55] = 0x09; // 0x0202
	l2capoutbuf[56] = 0x02;
	l2capoutbuf[57] = 0x02;
	l2capoutbuf[58] = 0x08;
	l2capoutbuf[59] = 0x40; //Keyboard (see Bluetooth Baseband)
	
	uint8_t tmp_size = 60;
/*
	//HIDVirtualCable (total 5)
	l2capoutbuf[tmp_size++] = 0x09; // 0x0204
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x04;
	l2capoutbuf[tmp_size++] = 0x28;
	l2capoutbuf[tmp_size++] = 0x01; //enable
*/	
//	uint8_t tmp_size = 65;

	//HIDBootDevice (total 5)
	l2capoutbuf[tmp_size++] = 0x09; // 0x020E
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x0E;
	l2capoutbuf[tmp_size++] = 0x28;
	l2capoutbuf[tmp_size++] = 0x00; // disenable
	
//	uint8_t tmp_size = 70;
/*
	//HIDProfileVersion (total 6)
	l2capoutbuf[tmp_size++] = 0x09; // 0x020B
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x0B;
	l2capoutbuf[tmp_size++] = 0x09; //Version 1.0
	l2capoutbuf[tmp_size++] = 0x01;
	l2capoutbuf[tmp_size++] = 0x00;
*/	
//	uint8_t tmp_size = 76;
/*
	//HIDParserVersion (total 6)
	l2capoutbuf[tmp_size++] = 0x09; // 0x0200
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x00;
	l2capoutbuf[tmp_size++] = 0x09; //Version 1.11
	l2capoutbuf[tmp_size++] = 0x01;
	l2capoutbuf[tmp_size++] = 0x11;
*/	
//	uint8_t tmp_size = 82;
/*
	//BluetoothProfileDescriptorList (total 13)
	l2capoutbuf[tmp_size++] = 0x09; // 0x0009
	l2capoutbuf[tmp_size++] = 0x00;
	l2capoutbuf[tmp_size++] = 0x09;
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x08;
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x06;
	l2capoutbuf[tmp_size++] = 0x19; // UUID=0x0011 HIDP
	l2capoutbuf[tmp_size++] = 0x00;
	l2capoutbuf[tmp_size++] = 0x11;
	l2capoutbuf[tmp_size++] = 0x09; // Version Number
	l2capoutbuf[tmp_size++] = 0x01;
	l2capoutbuf[tmp_size++] = 0x00;
*/
//	uint8_t tmp_size = 95;
	
	//HIDDescriptorList (total 56)
	l2capoutbuf[tmp_size++] = 0x09;
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x06;
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x33; //41Bytes
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x31; //39Bytes
	l2capoutbuf[tmp_size++] = 0x08;
	l2capoutbuf[tmp_size++] = 0x22; //Type = Report
	l2capoutbuf[tmp_size++] = 0x25;
	l2capoutbuf[tmp_size++] = 0x2D; //String 47Bytes
	l2capoutbuf[tmp_size++] = 0x05;
	l2capoutbuf[tmp_size++] = 0x01; //USAGE_PAGE(Generic Desktop)
	l2capoutbuf[tmp_size++] = 0x09;
	l2capoutbuf[tmp_size++] = 0x06; //USAGE(keyboard)
	l2capoutbuf[tmp_size++] = 0xA1;
	l2capoutbuf[tmp_size++] = 0x01; //COLLECTION(Application)
	l2capoutbuf[tmp_size++] = 0x05;
	l2capoutbuf[tmp_size++] = 0x07; //USAGE_PAGE(keyboard)
	//0x85, 0x01, //REPORT_ID(1) 10
	l2capoutbuf[tmp_size++] = 0x19;
	l2capoutbuf[tmp_size++] = 0xE0; //USAGE_MINIMUM(Keyboard LeftControl)E0
	l2capoutbuf[tmp_size++] = 0x29;
	l2capoutbuf[tmp_size++] = 0xE7; //USAGE_MAXIMUM(Keyboard Right GUI)E7
	l2capoutbuf[tmp_size++] = 0x15;
	l2capoutbuf[tmp_size++] = 0x00; //LOGICAL_MINIMUM(0)
	l2capoutbuf[tmp_size++] = 0x25;
	l2capoutbuf[tmp_size++] = 0x01; //LOGICAL_MAXIMUM(1)
	l2capoutbuf[tmp_size++] = 0x75;
	l2capoutbuf[tmp_size++] = 0x01; //REPORT_SIZE(1) 30
	l2capoutbuf[tmp_size++] = 0x95;
	l2capoutbuf[tmp_size++] = 0x08; //REPORT_COUNT(8)
	l2capoutbuf[tmp_size++] = 0x81;
	l2capoutbuf[tmp_size++] = 0x02; //INPUT(Data,Var,Abs)
	l2capoutbuf[tmp_size++] = 0x95;
	l2capoutbuf[tmp_size++] = 0x01; //REPORT_COUNT(1)
	l2capoutbuf[tmp_size++] = 0x75;
	l2capoutbuf[tmp_size++] = 0x08; //REPORT_SIZE(8)
	l2capoutbuf[tmp_size++] = 0x81;
	l2capoutbuf[tmp_size++] = 0x01; //INPUT(Cnst,Var,Abs) 20
	l2capoutbuf[tmp_size++] = 0x95;
	l2capoutbuf[tmp_size++] = 0x06; //REPORT_COUNT(6)
	l2capoutbuf[tmp_size++] = 0x75;
	l2capoutbuf[tmp_size++] = 0x08; //REPORT_SIZE(8)
	l2capoutbuf[tmp_size++] = 0x15;
	l2capoutbuf[tmp_size++] = 0x00; //LOGICAL_MINIMUM(0)
	l2capoutbuf[tmp_size++] = 0x25;
	l2capoutbuf[tmp_size++] = 0x65; //LOGICAL_MAXIMUM(101)
	l2capoutbuf[tmp_size++] = 0x05;
	l2capoutbuf[tmp_size++] = 0x07; //USAGE_PAGE(Key Code)
	l2capoutbuf[tmp_size++] = 0x19;
	l2capoutbuf[tmp_size++] = 0x00; //USAGE_MINIMUM(0)
	l2capoutbuf[tmp_size++] = 0x29;
	l2capoutbuf[tmp_size++] = 0x65; //USAGE_MAXIMUM(101)
	l2capoutbuf[tmp_size++] = 0x81;
	l2capoutbuf[tmp_size++] = 0x00; //INPUT(Data,Array) 28
	l2capoutbuf[tmp_size++] = 0xC0; //END_COLLECTION
/*
	//￼HIDReconnectInitiate (total 5)
	l2capoutbuf[tmp_size++] = 0x09; // 0x0205
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x05;
	l2capoutbuf[tmp_size++] = 0x28; //true
	l2capoutbuf[tmp_size++] = 0x01;
*/ 
/*
	//￼HIDNormallyConnectable (total 5)
	l2capoutbuf[tmp_size++] = 0x09; // 0x020D
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x0D;
	l2capoutbuf[tmp_size++] = 0x28; //true
	l2capoutbuf[tmp_size++] = 0x01;
*/ 
/*
	//￼LanguageBaseAttributeIDList (total 14)
	l2capoutbuf[tmp_size++] = 0x09; // 0x0006
	l2capoutbuf[tmp_size++] = 0x00;
	l2capoutbuf[tmp_size++] = 0x06;
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x09;
	l2capoutbuf[tmp_size++] = 0x09; //"en"(English)
	l2capoutbuf[tmp_size++] = 0x65;
	l2capoutbuf[tmp_size++] = 0x6E;
	l2capoutbuf[tmp_size++] = 0x09; //UTF-8 encoding
	l2capoutbuf[tmp_size++] = 0x00;
	l2capoutbuf[tmp_size++] = 0x6A;
	l2capoutbuf[tmp_size++] = 0x09; //PrimaryLanguageBaseID
	l2capoutbuf[tmp_size++] = 0x01;
	l2capoutbuf[tmp_size++] = 0x00;
*/ 
/*
	//￼HIDLANGIDBaseList (total 13)
	l2capoutbuf[tmp_size++] = 0x09; // 0x0207
	l2capoutbuf[tmp_size++] = 0x02;
	l2capoutbuf[tmp_size++] = 0x07;
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x08;
	l2capoutbuf[tmp_size++] = 0x35;
	l2capoutbuf[tmp_size++] = 0x06;
	l2capoutbuf[tmp_size++] = 0x09; //Language =English(U.S)
	l2capoutbuf[tmp_size++] = 0x04;
	l2capoutbuf[tmp_size++] = 0x09;
	l2capoutbuf[tmp_size++] = 0x09; //Bluetooth String Offset
	l2capoutbuf[tmp_size++] = 0x01;
	l2capoutbuf[tmp_size++] = 0x00;
*/
	l2capoutbuf[tmp_size] = 0x00; // No more data
	
#ifdef DEBUG
	Notify(PSTR("\r\ntmp_size: 0x"));
	PrintHex<uint8_t>(tmp_size);
#endif
	
	l2capoutbuf[4] = (tmp_size+1) - 5;  // Parameter Length
	l2capoutbuf[6] = (tmp_size+1) - (7+1);  // AttributeListByteCount
	l2capoutbuf[8] = (tmp_size+1) - (7+1+2);
	
	SDP_Command(l2capoutbuf, tmp_size+1);
}
/************************************************************/
/*                    HID Commands                          */
/************************************************************/
void BTHID::HID_Command(uint8_t* data, uint8_t nbytes) {
#ifdef DEBUG
	Notify(PSTR("\r\nhci_handle: "));
	PrintHex<uint8_t>(pBtd->hci_handle & 0xff);
	Notify(PSTR(" "));
	PrintHex<uint8_t>(((pBtd->hci_handle >> 8) & 0x0f) | 0x20);
	Notify(PSTR(" HID_Command: "));
	PrintHex<uint8_t>(m_control_scid[0]);
	Notify(PSTR(" "));
	PrintHex<uint8_t>(m_control_scid[1]);
#endif
		pBtd->L2CAP_Command(pBtd->hci_handle,data,nbytes,m_control_scid[0],m_control_scid[1]);//control
}

void BTHID::HID_Command_interrupt(uint8_t* data, uint8_t nbytes) {
#ifdef EXTRADEBUG
	Notify(PSTR("\r\nhci_handle: "));
	PrintHex<uint8_t>(pBtd->hci_handle & 0xff);
	Notify(PSTR(" "));
	PrintHex<uint8_t>(((pBtd->hci_handle >> 8) & 0x0f) | 0x20);
	Notify(PSTR(" HID_Command: "));
	PrintHex<uint8_t>(m_interrupt_scid[0]);
	Notify(PSTR(" "));
	PrintHex<uint8_t>(m_interrupt_scid[1]);
#endif
	pBtd->L2CAP_Command(pBtd->hci_handle,data,nbytes,m_interrupt_scid[0],m_interrupt_scid[1]);//interrupt
}
void BTHID::HID_Handshake(uint8_t result_code) {
#ifdef DEBUG
	Notify(PSTR("\r\nHID_Handshake"));
#endif
	uint8_t cmd_buf[1];
	cmd_buf[0] = (result_code |(0x00 << 4)) ;
	HID_Command(cmd_buf, 1);
//	connected = true;
}

void BTHID::HID_sendKeyCodes(uint8_t modifier, uint8_t keycode1, uint8_t keycode2, uint8_t keycode3, uint8_t keycode4, uint8_t keycode5, uint8_t keycode6) {
#ifdef DEBUG
	Notify(PSTR("\r\nHID_sendKeyCodes ModifierKey:0x"));
	PrintHex<uint8_t>(modifier);
	Notify(PSTR(" keycode1:0x"));
	PrintHex<uint8_t>(keycode1);
	Notify(PSTR(" KC2:0x"));
	PrintHex<uint8_t>(keycode2);
	Notify(PSTR(" KC3:0x"));
	PrintHex<uint8_t>(keycode3);
	Notify(PSTR(" KC4:0x"));
	PrintHex<uint8_t>(keycode4);
	Notify(PSTR(" KC5:0x"));
	PrintHex<uint8_t>(keycode5);
	Notify(PSTR(" KC6:0x"));
	PrintHex<uint8_t>(keycode6);
#endif
	uint8_t cmd_buf[9];
	cmd_buf[0] = 0xA1; 
//	cmd_buf[1] = 0x01; //ReportID(there is only one report and the Report ID field is omitted.)
	cmd_buf[1] = modifier; //Modifier keys
	cmd_buf[2] = 0x00; //Reserved
	cmd_buf[3] = keycode1; //Keycode 1
	cmd_buf[4] = keycode2; //Keycode 2
	cmd_buf[5] = keycode3; //Keycode 3
	cmd_buf[6] = keycode4; //Keycode 4
	cmd_buf[7] = keycode5; //Keycode 5
	cmd_buf[8] = keycode6; //Keycode 6
	
	HID_Command_interrupt(cmd_buf, 9);
}

void BTHID::HID_allKeyUp() {
#ifdef DEBUG
	Notify(PSTR("\r\nHID_allKeyUp"));
#endif
	
	HID_sendKeyCodes(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
}

void BTHID::HID_SendCharacter(uint8_t ascii) {
	uint8_t tmp_modifier = 0x00;
	// '!' to '&'
	if ( 0x20 < ascii && ascii < 0x27) { //0x21-0x26
		tmp_modifier = 0x02; //LeftShift
	}
	// '(' to '+'
	if ( 0x27 < ascii && ascii < 0x2C) { //0x28-0x2B
		tmp_modifier = 0x02; //LeftShift
	}
	// '?' to 'Z'
	if ( 0x3E < ascii && ascii < 0x5B) { //0x41-0x5A
		tmp_modifier = 0x02; //LeftShift
	}
	// '^' to '_'
	if ( 0x5D < ascii && ascii < 0x60) { //0x5E-0x5F
		tmp_modifier = 0x02; //LeftShift
	}
	// '{' to '~'
	if ( 0x7A < ascii && ascii < 0x7F) { //0x7B-0x7E
		tmp_modifier = 0x02; //LeftShift
	}
#ifdef DEBUG
	if (tmp_modifier == 0x02) {
			Notify(PSTR("\r\n//Shift//"));
	}
#endif
	
	HID_sendKeyCodes(tmp_modifier, HID_AsciitoKeycode(ascii), 0x00, 0x00, 0x00, 0x00, 0x00);
	delay(5);
	HID_allKeyUp();
	delay(5);
}

uint8_t BTHID::HID_AsciitoKeycode(uint8_t ascii) {
#ifdef DEBUG
	Notify(PSTR("\r\nHID_AsciitoKeycode: 0x"));
	PrintHex<uint8_t>(ascii);
	Notify(PSTR(" -> 0x"));
	PrintHex<uint8_t>(pgm_read_byte(&asciiandkeycode[(uint8_t) ascii]));
#endif
	
	return pgm_read_byte(&asciiandkeycode[(uint8_t) ascii]);
}

void BTHID::HID_sendString(char *str) {
	uint8_t tmp_counter = 0;
	while (*str != '\0') {
#ifdef DEBUG
		Notify(PSTR("\r\nHID_sendString: "));
		PrintHex<uint8_t>(*str);
#endif
		HID_SendCharacter(*str);
		*str++;
		tmp_counter++;
		//Ignore over 32chars
		if (tmp_counter == 31)
			break;
	}
}

void BTHID::HID_sendInteger(int16_t integer) {
	char tmp_sNum[7];
	sprintf(tmp_sNum, "%d", integer);
	HID_sendString(tmp_sNum);
}

void BTHID::HID_sendFloat(float myfloat) {
	int d1 = myfloat;            // Get the integer part (678).
	float f2 = myfloat - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
	int d2 = trunc(f2 * 10000);

	char tmp_sFloat[7];
	sprintf(tmp_sFloat, "%d.%04d", d1, d2);
	HID_sendString(tmp_sFloat);
}
