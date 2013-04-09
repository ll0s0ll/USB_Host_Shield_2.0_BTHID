
#include <BTHID.h>
USB Usb;
BTD Btd(&Usb);
BTHID bthid(&Btd);

void setup() {
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nBluetooth HID"));
  Btd.btdName = "Arduino";  // Set device name
}
  
void loop() {
  
  if (Serial.available() > 0) {
    uint8_t incomingbuf = Serial.read();
    Serial.print(F("\r\n[INCOMING : 0x"));
    Serial.print(incomingbuf, HEX);
    Serial.print(F("]"));
    
     // '1'
    if (incomingbuf == 0x31) {         // 0x32 = '1'
      char str[] = "Hello World!";
      //char str[32] = "0123456789012345678901234567890";
      bthid.HID_sendString(str);       // Send String
    // '2'
    } else if (incomingbuf == 0x32) {  // 0x32 = '2'
      bthid.HID_sendInteger(12345);    // Send integer
    // '3'
    } else if (incomingbuf == 0x33) {  // 0x33 = '3'
      bthid.HID_sendFloat(3.1415);     // Send float
    // '4'
    } else if (incomingbuf == 0x34) {  // 0x34 = '4'
      bthid.HID_sendKeyCodes(0x00, 0x91,0x00,0x00,0x00,0x00,0x00);  // 0x91 = "EISU"(for Japanese)
      delay(5);
      bthid.HID_allKeyUp();            // Keyup
      delay(5);
    // '5'
    } else if (incomingbuf == 0x35) {  // 0x35 = '5'
      bthid.HID_sendKeyCodes(0x00, 0x90,0x00,0x00,0x00,0x00,0x00);  // 0x90 = "KANA"(for Japanese)
      delay(5);
      bthid.HID_allKeyUp();            // Keyup
      delay(5);
    // 'Q'
    } else if (incomingbuf == 0x51) {  // 0x51 = 'Q'
      bthid.disconnect();              // Disconnect
    // 'D'
    } else if (incomingbuf == 0x44) {  // 0x44 = 'D'
      bthid.HID_SendCharacter(0x7F);   // 0x7F = Delete
    // 'R'
    } else if (incomingbuf == 0x52) {  // 0x52 = 'R'
      bthid.HID_SendCharacter(0x0D);   // 0x0D = Return
    } else {
      bthid.HID_SendCharacter(incomingbuf);  // Send ASCII
    }
  }
  Usb.Task();
}

