////////////////////////////////////////////////////////////////////////
//
// M5 Faces Keyboard & Game Pad Emulator
// MSX0 KEYPAD mode Test
// Copyright 2023 @V9938
//	
//	23/04/27 V1.0		1st version
//
////////////////////////////////////////////////////////////////////////
// 下記ライブラリを使用しています。
// USB Host Shield library Version 2.0
// https://chome.nerpa.tech/arduino_usb_host_shield_projects/
////////////////////////////////////////////////////////////////////////


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

//	if (Usb.Init() == -1){					//USBユニットが無い場合の処理
//		#ifdef DEBUG_PRINT 
//		Serial.println("OSC did not start.");
//		#endif
//		padMode = 1;						//PADモードに移行
//		padData = 0xff;						//PADモードは初期値は7F以上返す必要がある。
//	}

//	delay( 200 );							//キーボード初期化待ち
//	HidKeyboard.SetReportParser(0, &Prs);


}

void loop()
{

// ループ処理

	if (hadPressed ==0){						//KEYBUFFERが空いている時のみ有効
		strBufEnd = 0;
		strBufPointer = 0;
		KeyDataStrUpdate("MSX0 ");
	}

}
