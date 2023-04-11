////////////////////////////////////////////////////////////////////////
//
// M5 Faces Keyboard & Game Pad Emulator
// MSX0 USB-Keyboard MBUS Stack
// Copyright 2023 @V9938
//	
//	23/04/06 V1.0		1st version
//
////////////////////////////////////////////////////////////////////////
// 下記ライブラリを使用しています。
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


// UARTにデバッグMessageを表示するか（通常は無効)
//#define DEBUG_PRINT 1
// I2Cのアドレス
#define FACES_KEYBOARD_I2C_ADDR 0x08

// M5キー割り込みの出力のポート
#define IRQ_1 Set_Bit(PORTD,4)
#define IRQ_0 Clr_Bit(PORTD,4)

// グローバル変数
unsigned char hadPressed = 0;			// ボタン押されたかフラグ
unsigned char stringBuffer[128];		// KEYBOARD入力値を貯めるバッファー
unsigned char strBufEnd;				// バッファー最終値
unsigned char strBufPointer;			// バッファー現在値
unsigned char padMode;					// PADモードかどうかのフラグ
unsigned char padData;					// I2C送信値 0x7f以上をMSX0起動時に送るとGAMEPADモードになる
unsigned char tDataOld;					// PADの前入力値


// I2C 0x08の割り込み処理
void requestEvent()
{

// キーが押された履歴チェック
	if (hadPressed == 1){
		if (padMode == 0){
//Keyboardモード：KeyBufferに貯めたデータを順次送る
			Wire.write(stringBuffer[strBufPointer]);
			strBufPointer++;
			if (strBufEnd == strBufPointer) {
				hadPressed = 0;
				IRQ_1;
			}
		}else{
//GamePadモード：入力値をそのまま送る
			Wire.write(padData);
			hadPressed = 0;
			IRQ_1;
		}

	}else{
// M5キー割り込みが無い場合は通常ここからデータが返される
		Wire.write(padData);
	}	
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
void KeyDataUpdate(unsigned char c){
// キーボードバッファーへの一文字入力
	stringBuffer[strBufEnd] = c;
	strBufEnd++;						//toDO:Overflow対策

	hadPressed = 1;
	IRQ_0;

}
void KeyDataStrUpdate(String str){
// キーボードバッファーへの文字列入力(MAX16文字)

	unsigned char chrs[16];
	int i;
	str.toCharArray(chrs,16);  					//Strクラスから変換

	for (i=0;i<str.length();i++){				//文字コピー
		if (chrs[i] == 0x0a){
			stringBuffer[strBufEnd] = 0x0d;
		}else{
			stringBuffer[strBufEnd] = chrs[i];
		}
		strBufEnd++;
	}

	hadPressed = 1;								//キーデータあり
	IRQ_0;										//キー入力をM5に通知
}



void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
// キーボードが押された場合に実行される処理

// キーデータについては、別データを参照
//	USBキーボード(JP)とM5 Facesのキーコード変換テーブル (シフトキーなし）
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

//	USBキーボード(JP)とM5 Facesのキーコード変換テーブル (シフトキー有り）
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

	uint8_t c;					// 変換後のコード値
	MODIFIERKEYS m;				// Shift/Alt/Ctrlの通知

	*((uint8_t*)&m) = mod;


	#ifdef DEBUG_PRINT 
	Serial.print("DN ");
	PrintKey(mod, key);
	#endif
// USキー配置なのでローカルテーブルに変更
//  uint8_t c = OemToAscii(mod, key);

	if ((m.bmLeftShift  == 1)|(m.bmRightShift  == 1)){
		c = KeyboardTablewShift[key];
	}else{
		c = KeyboardTable[key];
	}
//10キーが使われた時のNUMLOCK処理

	if ((key >= 0x54) && (key <= 0x62)){
		if (kbdLockingKeys.kbdLeds.bmNumLock == 1) {		//NUMLOCKの状態で入力値を変える
			c = KeyboardTablewShift[key];
		}else{
			c = KeyboardTable[key];
		}
	}
// ctrl+cの例外処理
	if ((key == 0x06) || (key == 0x44)){
		if ((m.bmRightCtrl  == 1)|(m.bmLeftCtrl  == 1)){
			c = 0xbc;
		}
	}



// ファンクションキーの処理
// 文字列入力の処理（16文字までOK）
// ここで処理する場合は変換後のコード値は0x00である必要あり
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

// I2Cへデータをセット
	if (c){
		KeyDataUpdate(c);
		OnKeyPressed(c);

	}
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {
// SHIFT/ALT/CTRLの状態変化があった場合に呼ばれる
// 他のキーと併用するキーはここで押された事を調べる必要があります。

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
	padMode = 0;
	padData = 0x00;
	strBufEnd  = 0;
	strBufPointer = 0;
	hadPressed = 0;

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
		padMode = 1;						//PADモードに移行
		padData = 0xff;						//PADモードは初期値は7F以上返す必要がある。
	}

	delay( 200 );							//キーボード初期化待ち
	HidKeyboard.SetReportParser(0, &Prs);


}

void loop()
{

// ループ処理
	unsigned char tData = 0xff;

	Usb.Task();

	if (hadPressed == 0) {							// Bufferのポインタリセット処理
		strBufEnd  = 0;								// 空になったら初期位置に戻す
		strBufPointer = 0;
	}

	tData = 0xff;										// JOYSTICKポート入力
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


	if( Usb.getUsbTaskState() != USB_STATE_RUNNING ) { //USBが抜いているときはJOYSTICKモードに移行
		padMode = 1;

		if (tData != tDataOld){							//KEYDATAの更新があった時のみ通知
			padData = tData;
			hadPressed = 1;
			IRQ_0;
		}

	}else{
		padData = 0x00;									//USBキーボードがつながれている時
		padMode = 0;

		if (tData != tDataOld){
			if (hadPressed ==0){						//KEYBUFFERが空いている時のみ有効
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
