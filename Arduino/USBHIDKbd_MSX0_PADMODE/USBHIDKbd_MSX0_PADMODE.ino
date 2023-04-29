////////////////////////////////////////////////////////////////////////
//
// M5 Faces Keyboard & Game Pad Emulator
// MSX0 USB-Keyboard MBUS Stack (GAMEPAD Mode Only Version)
// Copyright 2023 @V9938
//	
//	23/04/30 V1.0		1st version
//
////////////////////////////////////////////////////////////////////////
// 下記ライブラリを使用しています。
// USB Host Shield library Version 2.0
// https://chome.nerpa.tech/arduino_usb_host_shield_projects/
////////////////////////////////////////////////////////////////////////

// KEY Assign /////////////////////////////////////////////////////////
// Cursor UP/DOWN/LEFT/RIGHT        GAMEPAD: UP/DOWN/LEFT/RIGHT
// SPACE                            GAMEPAD: A
// Ctrl                             GAMEPAD: B
// F1                               GAMEPAD: SELECT
// F2                               GAMEPAD: START
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


// UARTにデバッグMessageを表示するか（通常は無効)
//#define DEBUG_PRINT 1
// I2Cのアドレス
#define FACES_KEYBOARD_I2C_ADDR 0x08

// M5キー割り込みの出力のポート
#define IRQ_1 Set_Bit(PORTD,4)
#define IRQ_0 Clr_Bit(PORTD,4)

// M5 Faces Game Padモードでのビット配置
#define M5Faces_Up		0xfe
#define M5Faces_Down	0xfd
#define M5Faces_Left	0xfb
#define M5Faces_Right	0xf7
#define M5Faces_A		0xef
#define M5Faces_B		0xdf
#define M5Faces_Select	0xbf
#define M5Faces_Start	0x7f


// グローバル変数
unsigned char padData;					// I2C送信値 0x7f以上をMSX0起動時に送るとGAMEPADモードになる
bool ctrlData;							// Ctrl のOn/Off Flag保存用

// I2C 0x08の割り込み処理
void requestEvent()
{

//GamePadモード：入力値をそのまま送る
	Wire.write(padData);
	if (padData == 0xff)	IRQ_1;		// キー入力が無い場合はキー受け取り要求をしない
//	Serial.println(padData);

	
}
void MakePadDN(unsigned char data){
	padData = padData & data;
}

void MakePadUP(unsigned char data){
	padData = padData | (0xff ^ data);
}

// Class定義
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
// キーボード入力の表示（デバッグ用)
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


void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
// キーボードが押された場合に実行される処理
	if (key == 0x52) MakePadDN(M5Faces_Up);
	if (key == 0x51) MakePadDN(M5Faces_Down);
	if (key == 0x50) MakePadDN(M5Faces_Left);
	if (key == 0x4f) MakePadDN(M5Faces_Right);
	if (key == 0x2C) MakePadDN(M5Faces_A);		// SPACE

	if (key == 0x3A) MakePadDN(M5Faces_Select);		// Select [F1] (MSX0ではnot Assign)
	if (key == 0x3B) MakePadDN(M5Faces_Start);		// Start  [F2] (MSX0ではnot Assign) 

	IRQ_0;										//キー入力をM5に通知

}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {
// SHIFT/ALT/CTRLの状態変化があった場合に呼ばれる
// 他のキーと併用するキーはここで押された事を調べる必要があります。

	MODIFIERKEYS beforeMod;
	*((uint8_t*)&beforeMod) = before;

	MODIFIERKEYS afterMod;
	*((uint8_t*)&afterMod) = after;

	if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
		ctrlData = !ctrlData;
		#ifdef DEBUG_PRINT 
		Serial.println("LeftCtrl changed");
		#endif
	}

	if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
		ctrlData = !ctrlData;
		#ifdef DEBUG_PRINT 
		Serial.println("RightCtrl changed");
		#endif
	}

	if (ctrlData == true) MakePadDN(M5Faces_B);
	else MakePadUP(M5Faces_B);
	IRQ_0;										//キー入力をM5に通知
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
	if (key == 0x52) MakePadUP(M5Faces_Up);
	if (key == 0x51) MakePadUP(M5Faces_Down);
	if (key == 0x50) MakePadUP(M5Faces_Left);
	if (key == 0x4f) MakePadUP(M5Faces_Right);
	if (key == 0x2C) MakePadUP(M5Faces_A);		// SPACE

	if (key == 0x3A) MakePadDN(M5Faces_Select);	// Select [F1](MSX0ではnot Assign)
	if (key == 0x3B) MakePadDN(M5Faces_Start);	// Start  [F2](MSX0ではnot Assign) 

	IRQ_0;										//キー入力をM5に通知

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
//ポート設定

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

//グローバル変数初期化
	ctrlData = 0;
	padData = 0xff;

//I2C有効、割り込み使用
	Wire.begin(FACES_KEYBOARD_I2C_ADDR);
	Wire.onRequest(requestEvent);

//UART一応有効化(ファーム書き換え時に困るため）
	Serial.begin( 115200 );

	#ifdef DEBUG_PRINT 
	#if !defined(__MIPSEL__)
	while (!Serial); // Leonardo用のWait処理、USBシリアル接続完了まで待たされる。
	#endif
	Serial.println("Start");
	#endif

	if (Usb.Init() == -1){					//USBユニットが無い場合の処理
		#ifdef DEBUG_PRINT 
		Serial.println("OSC did not start.");
		#endif
	}

	delay( 200 );							//キーボード初期化待ち
	HidKeyboard.SetReportParser(0, &Prs);


}

void loop()
{


	Usb.Task();


}
