////////////////////////////////////////////////////////////////////////
//
// M5 Faces Keyboard & Game Pad Emulator
// MSX0 USB-Keyboard MBUS Stack
// Copyright 2023 @V9938
//	
//	23/04/06 V1.0		1st version
//
////////////////////////////////////////////////////////////////////////
// ���L���C�u�������g�p���Ă��܂��B
// USB Host Shield library Version 2.0
// https://chome.nerpa.tech/arduino_usb_host_shield_projects/
////////////////////////////////////////////////////////////////////////


#include <hidboot.h>
#include <usbhub.h>
#include <Wire.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#define Set_Bit(val, bitn)    (val |=(1<<(bitn)))
#define Clr_Bit(val, bitn)     (val&=~(1<<(bitn)))
#define Get_Bit(val, bitn)    (val &(1<<(bitn)) )


// UART�Ƀf�o�b�OMessage��\�����邩�i�ʏ�͖���)
//#define DEBUG_PRINT 1
// I2C�̃A�h���X
#define FACES_KEYBOARD_I2C_ADDR 0x08

// M5�L�[���荞�݂̏o�͂̃|�[�g
#define IRQ_1 Set_Bit(PORTD,4)
#define IRQ_0 Clr_Bit(PORTD,4)

// �O���[�o���ϐ�
unsigned char hadPressed = 0;			// �{�^�������ꂽ���t���O
unsigned char stringBuffer[128];		// KEYBOARD���͒l�𒙂߂�o�b�t�@�[
unsigned char strBufEnd;				// �o�b�t�@�[�ŏI�l
unsigned char strBufPointer;			// �o�b�t�@�[���ݒl
unsigned char padMode;					// PAD���[�h���ǂ����̃t���O
unsigned char padData;					// I2C���M�l 0x7f�ȏ��MSX0�N�����ɑ����GAMEPAD���[�h�ɂȂ�
unsigned char tDataOld;					// PAD�̑O���͒l


// I2C 0x08�̊��荞�ݏ���
void requestEvent()
{

// �L�[�������ꂽ�����`�F�b�N
	if (hadPressed == 1){
		if (padMode == 0){
//Keyboard���[�h�FKeyBuffer�ɒ��߂��f�[�^����������
			Wire.write(stringBuffer[strBufPointer]);
			strBufPointer++;
			if (strBufEnd == strBufPointer) {
				hadPressed = 0;
				IRQ_1;
			}
		}else{
//GamePad���[�h�F���͒l�����̂܂ܑ���
			Wire.write(padData);
			hadPressed = 0;
			IRQ_1;
		}

	}else{
// M5�L�[���荞�݂������ꍇ�͒ʏ킱������f�[�^���Ԃ����
		Wire.write(padData);
	}	
}

// Class��`
class KbdRptParser : public KeyboardReportParser
{
	void PrintKey(uint8_t mod, uint8_t key);

protected:
	void OnControlKeysChanged(uint8_t before, uint8_t after);

	void OnKeyDown	(uint8_t mod, uint8_t key);
	void OnKeyUp	(uint8_t mod, uint8_t key);
	void OnKeyPressed(uint8_t key);
};


void KbdRptParser::PrintKey(uint8_t m, uint8_t key)
{
// �L�[�{�[�h���͂̕\���i�f�o�b�O�p)
	#ifdef DEBUG_PRINT 
	MODIFIERKEYS mod;
	*((uint8_t*)&mod) = m;
	Serial.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
	Serial.print((mod.bmLeftShift  == 1) ? "S" : " ");
	Serial.print((mod.bmLeftAlt    == 1) ? "A" : " ");
	Serial.print((mod.bmLeftGUI    == 1) ? "G" : " ");

	Serial.print(" >");
	PrintHex<uint8_t>(key, 0x80);
	Serial.print("< ");

	Serial.print((mod.bmRightCtrl   == 1) ? "C" : " ");
	Serial.print((mod.bmRightShift  == 1) ? "S" : " ");
	Serial.print((mod.bmRightAlt    == 1) ? "A" : " ");
	Serial.println((mod.bmRightGUI    == 1) ? "G" : " ");
	#endif

};
void KeyDataUpdate(unsigned char c){
// �L�[�{�[�h�o�b�t�@�[�ւ̈ꕶ������
	stringBuffer[strBufEnd] = c;
	strBufEnd++;						//toDO:Overflow�΍�

	hadPressed = 1;
	IRQ_0;

}
void KeyDataStrUpdate(String str){
// �L�[�{�[�h�o�b�t�@�[�ւ̕��������(MAX16����)

	unsigned char chrs[16];
	int i;
	str.toCharArray(chrs,16);  					//Str�N���X����ϊ�

	for (i=0;i<str.length();i++){				//�����R�s�[
		if (chrs[i] == 0x0a){
			stringBuffer[strBufEnd] = 0x0d;
		}else{
			stringBuffer[strBufEnd] = chrs[i];
		}
		strBufEnd++;
	}

	hadPressed = 1;								//�L�[�f�[�^����
	IRQ_0;										//�L�[���͂�M5�ɒʒm
}



void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
// �L�[�{�[�h�������ꂽ�ꍇ�Ɏ��s����鏈��

// �L�[�f�[�^�ɂ��ẮA�ʃf�[�^���Q��
//	USB�L�[�{�[�h(JP)��M5 Faces�̃L�[�R�[�h�ϊ��e�[�u�� (�V�t�g�L�[�Ȃ��j
	const uint8_t KeyboardTable[256]       PROGMEM = { 
//         0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
		0x00, 0x00, 0x00, 0x00,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l', 	// 0x00
		'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2', 		// 0x10
		 '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', 0x0d, 0x1b, 0x08, 0x09, 0x20, 0x2d, 0xde, 0x00, 	// 0x20
		0xdb, 0x00, 0xdd, 0x3b, 0x3a, 0x00, 0x2c, 0x2e, 0x2f, 0xf2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x30
		0x00, 0x00, 0x00, 0x00, 0x7b, 0xf4, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0xf7, 	// 0x40
		0xfb, 0xfd, 0xfe, 0x00, 0x2f, 0x2a, 0x2d, 0x2b, 0x0d, 0x00, 0xfd, 0x00, 0xfb, 0x00, 0xf7, 0x00, 	// 0x50
		0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x60 (TENKEY NUMLOCK OFF)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x70
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0xf3, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x80
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x90
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xA0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xB0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xC0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xD0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xE0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   	// 0xF0
	};

//	USB�L�[�{�[�h(JP)��M5 Faces�̃L�[�R�[�h�ϊ��e�[�u�� (�V�t�g�L�[�L��j
	const uint8_t KeyboardTablewShift[256] PROGMEM = { 
//        0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
		0x00, 0x00, 0x00, 0x00,  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L', 	// 0x00
		 'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z', 0x00, 0x22, 	// 0x10
		0x23, 0x24, 0x00, 0x26, 0x27, 0x28, 0x29,  '0', 0x0d, 0x1b, 0x08, 0x09, 0x20, 0x2d, 0xde, 0x00, 	// 0x20
		0xdb, 0x00, 0xdd, 0x2b, 0x2a, 0x00, 0x3c, 0x3e, 0x00, 0xf2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x30
		0x00, 0x00, 0x00, 0x00, 0x7b, 0xf4, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0xf7, 	// 0x40
		0xfb, 0xfd, 0xfe, 0x00, 0x2f, 0x2A, 0x2D, 0x2B, 0x0d,  '1',  '2',  '3',  '4',  '5',  '6',  '7', 	// 0x50 54-62 tenkey
		 '8',  '9',  '0', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x60
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x70
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0xf3, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x80
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0x90
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xA0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xB0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xC0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xD0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// 0xE0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   	// 0xF0
	};

	uint8_t c;					// �ϊ���̃R�[�h�l
	MODIFIERKEYS m;				// Shift/Alt/Ctrl�̒ʒm

	*((uint8_t*)&m) = mod;


	#ifdef DEBUG_PRINT 
	Serial.print("DN ");
	PrintKey(mod, key);
	#endif
// US�L�[�z�u�Ȃ̂Ń��[�J���e�[�u���ɕύX
//  uint8_t c = OemToAscii(mod, key);

	if ((m.bmLeftShift  == 1)|(m.bmRightShift  == 1)){
		c = KeyboardTablewShift[key];
	}else{
		c = KeyboardTable[key];
	}
//10�L�[���g��ꂽ����NUMLOCK����

	if ((key >= 0x54) && (key <= 0x62)){
		if (kbdLockingKeys.kbdLeds.bmNumLock == 1) {		//NUMLOCK�̏�Ԃœ��͒l��ς���
			c = KeyboardTablewShift[key];
		}else{
			c = KeyboardTable[key];
		}
	}
// ctrl+c�̗�O����
	if ((key == 0x06) || (key == 0x44)){
		if ((m.bmRightCtrl  == 1)|(m.bmLeftCtrl  == 1)){
			c = 0xbc;
		}
	}



// �t�@���N�V�����L�[�̏���
// ��������͂̏����i16�����܂�OK�j
// �����ŏ�������ꍇ�͕ϊ���̃R�[�h�l��0x00�ł���K�v����
	switch(key)
	{
		case 0x3a:      //F1 key
		KeyDataStrUpdate("color ");
		break;

		case 0x3b:      //F2 key
		KeyDataStrUpdate("auto ");
		break;

		case 0x3c:      //F3 key
		KeyDataStrUpdate("goto ");
		break;

		case 0x3d:      //F4 key
		KeyDataStrUpdate("list ");
		break;

		case 0x3e:      //F5 key
		KeyDataStrUpdate("run\n");
		break;

		case 0x3f:      //F6 key
		KeyDataStrUpdate("color 15,4,7\n");
		break;

		case 0x40:      //F7 key
		KeyDataStrUpdate("load\"");
		break;

		case 0x41:      //F8 key
		KeyDataStrUpdate("cont\n");
		break;

		case 0x42:      //F9 key
		KeyDataStrUpdate("list.\n");
		break;
		case 0x43:      //F10 key
		KeyDataStrUpdate("run");
		break;
	}

// I2C�փf�[�^���Z�b�g
	if (c){
		KeyDataUpdate(c);
		OnKeyPressed(c);

	}
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {
// SHIFT/ALT/CTRL�̏�ԕω����������ꍇ�ɌĂ΂��
// ���̃L�[�ƕ��p����L�[�͂����ŉ����ꂽ���𒲂ׂ�K�v������܂��B

	MODIFIERKEYS beforeMod;
	*((uint8_t*)&beforeMod) = before;

	MODIFIERKEYS afterMod;
	*((uint8_t*)&afterMod) = after;

	if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
		if(afterMod.bmLeftCtrl) KeyDataUpdate(0x11);
		#ifdef DEBUG_PRINT 
		Serial.println("LeftCtrl changed");
		#endif
	}
	if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
		if(afterMod.bmLeftShift) KeyDataUpdate(0x10);
		#ifdef DEBUG_PRINT 
		Serial.println("LeftShift changed");
		#endif
	}
	if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
		if(afterMod.bmLeftAlt) KeyDataUpdate(0xf1);
		#ifdef DEBUG_PRINT 
		Serial.println("LeftAlt changed");
		#endif
	}
	if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
		#ifdef DEBUG_PRINT 
		Serial.println("LeftGUI changed");
		#endif
	}

	if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
		if (afterMod.bmRightCtrl) KeyDataUpdate(0x11);
		#ifdef DEBUG_PRINT 
		Serial.println("RightCtrl changed");
		#endif
	}
	if (beforeMod.bmRightShift != afterMod.bmRightShift) {
		if (afterMod.bmRightShift) KeyDataUpdate(0x10);
		#ifdef DEBUG_PRINT 
		Serial.println("RightShift changed");
		#endif
	}
	if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
		if (afterMod.bmRightAlt) KeyDataUpdate(0xf1);
		#ifdef DEBUG_PRINT 
		Serial.println("RightAlt changed");
		#endif
	}
	if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
		#ifdef DEBUG_PRINT 
		Serial.println("RightGUI changed");
		#endif
	}

}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
	#ifdef DEBUG_PRINT 
	Serial.print("UP ");
	PrintKey(mod, key);
	#endif
}

void KbdRptParser::OnKeyPressed(uint8_t key)
{
	#ifdef DEBUG_PRINT 
	Serial.print("ASCII: ");
	Serial.println((char)key);
	#endif
};

USB     Usb;
//USBHub     Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);

KbdRptParser Prs;

void setup()
{
//�|�[�g�ݒ�

	DDRB = DDRB | 0x10;			//PB4 OUTPUT (USB RST)
	DDRB = DDRB & 0xDF;			//PB5 INPUT (INT) 
	PORTB = PORTB | 0x20;		//PB5 Pullup
	PORTB = PORTB & 0xE0;		//PB4 Low->USB Unit Reset

	DDRD = DDRD|0x90;			//PD7 DSUB COM PD4: OUTPUT (M5 KEYISR)
	DDRD = DDRD&0xFC;			//PD0-1:INPUT (TRIG0/1)
	PORTD = PORTD | 0x13;		//PD4:H ,PD1-0 Pull UP
	PORTD = PORTD & 0x7f;		//PD7:L4

	DDRF = DDRF & 0x0F;			//PF4-7 INPUT (DSUB)
	PORTF = PORTF | 0xF0;		//PF4-7 Pullup

	PORTB = PORTB | 0x10;		//PB4 HIGH

//�O���[�o���ϐ�������
	padMode = 0;
	padData = 0x00;
	strBufEnd  = 0;
	strBufPointer = 0;
	hadPressed = 0;

//I2C�L���A���荞�ݎg�p
	Wire.begin(FACES_KEYBOARD_I2C_ADDR);
	Wire.onRequest(requestEvent);

//UART�ꉞ�L����(�t�@�[�������������ɍ��邽�߁j
	Serial.begin( 115200 );

	#ifdef DEBUG_PRINT 
	#if !defined(__MIPSEL__)
	while (!Serial); // Leonardo�p��Wait�����AUSB�V���A���ڑ������܂ő҂������B
	#endif
	Serial.println("Start");
	#endif

	if (Usb.Init() == -1){					//USB���j�b�g�������ꍇ�̏���
		#ifdef DEBUG_PRINT 
		Serial.println("OSC did not start.");
		#endif
		padMode = 1;						//PAD���[�h�Ɉڍs
		padData = 0xff;						//PAD���[�h�͏����l��7F�ȏ�Ԃ��K�v������B
	}

	delay( 200 );							//�L�[�{�[�h�������҂�
	HidKeyboard.SetReportParser(0, &Prs);


}

void loop()
{

// ���[�v����
	unsigned char tData = 0xff;

	Usb.Task();

	if (hadPressed == 0) {							// Buffer�̃|�C���^���Z�b�g����
		strBufEnd  = 0;								// ��ɂȂ����珉���ʒu�ɖ߂�
		strBufPointer = 0;
	}

	tData = 0xff;										// JOYSTICK�|�[�g����
	if ((PINF & 0x30) == 0) tData = tData & 0xBf; 		// Start
	else {
		if ((PINF & 0x10) == 0) tData = tData & 0xfe; 	//UP
		if ((PINF & 0x20) == 0) tData = tData & 0xfd; 	//DOWN
	}
	if ((PINF & 0xC0) == 0) tData = tData & 0x7f; 		//Select
	else {
		if ((PINF & 0x40) == 0) tData = tData & 0xfb; 	//LEFT
		if ((PINF & 0x80) == 0) tData = tData & 0xf7; 	//RIGHT
	}

	if ((PIND & 0x04) == 0) tData = tData & 0xef; 		//A
	if ((PIND & 0x08) == 0) tData = tData & 0xdf; 		//B


	if( Usb.getUsbTaskState() != USB_STATE_RUNNING ) { //USB�������Ă���Ƃ���JOYSTICK���[�h�Ɉڍs
		padMode = 1;

		if (tData != tDataOld){							//KEYDATA�̍X�V�����������̂ݒʒm
			padData = tData;
			hadPressed = 1;
			IRQ_0;
		}

	}else{
		padData = 0x00;									//USB�L�[�{�[�h���Ȃ���Ă��鎞
		padMode = 0;

		if (tData != tDataOld){
			if (hadPressed ==0){						//KEYBUFFER���󂢂Ă��鎞�̂ݗL��
				if ((tData & 0x01) == 0) KeyDataUpdate(0xfe);	//UP
				if ((tData & 0x02) == 0) KeyDataUpdate(0xfd);	//DOWN
				if ((tData & 0x04) == 0) KeyDataUpdate(0xfb);	//LEFT
				if ((tData & 0x08) == 0) KeyDataUpdate(0xf7);	//RIGHT
				if ((tData & 0x10) == 0) KeyDataUpdate(0x20);	//A
				if ((tData & 0x20) == 0) KeyDataUpdate(0x6d);	//B
				if ((tData & 0x40) == 0) KeyDataUpdate(0xf4);	//SELECT
				if ((tData & 0x80) == 0) KeyDataUpdate(0x0d);	//START
			}
		}


	}
	tDataOld = tData;

}
