////////////////////////////////////////////////////////////////////////
//
// M5 Faces Keyboard & Game Pad Emulator
// MSX0 USB-GamePad MBUS Stack
// Copyright 2023 @V9938
//	
//	23/04/29 V1.0		1st version
//	23/05/06 V1.1		Add some supported controllers
//
////////////////////////////////////////////////////////////////////////
// 下記ライブラリを使用しています。
// USB Host Shield library Version 2.0
// https://chome.nerpa.tech/arduino_usb_host_shield_projects/
////////////////////////////////////////////////////////////////////////

#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include <Wire.h>

// UARTにデバッグMessageを表示するか（通常は無効)
//#define DEBUG_PRINT 
//#define USB_RAW_DISPLAY

#define FACES_KEYBOARD_I2C_ADDR 0x08
#define FACES_GAMEPAD_MODE

#define Set_Bit(val, bitn)    (val |=(1<<(bitn)))
#define Clr_Bit(val, bitn)     (val&=~(1<<(bitn)))
#define Get_Bit(val, bitn)    (val &(1<<(bitn)) )

//KEY PORTB
//IRQ PC0
#define IRQ_1 Set_Bit(PORTD,4)
#define IRQ_0 Clr_Bit(PORTD,4)

unsigned char JoyCl = 0;

// Support Controller
#define JoyClMax			VID0F0DPID00ED	//最終コントローラID

#define UNKNOWN_VIDPID		255				//非対応コントーラID

#define VID0CA3PID0025		0				//MD PAD USB (MK-16500)
#define VID0CA3PID0024		1				//MD 6B PAD USB (HAA-2522)

// Byte3 X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte4 Y方向 UP   0x00-0x7f-0xff DOWN
// Byte5 0x4F A / 0x2F B / 0x8F X / 0x1F Y 
// Byte6 0x02 C / 0x20 Start / 0x01 Z / 0x10 Mode

#define VID0F0DPID0138		2				//KONAMI PCEmini Controller(HT-002)

// Byte0 II 0x02 / I 0x04 
// Byte1 Select 0x01 / Start 0x02
// Byte2 HATSW (上右)12345678(上) 0x0f (NoKey)

#define VID0810PIDE501		3				//NB NES USB Controller
// Byte3 X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte4 Y方向 UP   0x00-0x7f-0xff DOWN
// Byte5 0x1F A / 0x2F B
// Byte6 0x20 Start / 0x10 Select

#define VID054CPID0CDA		4				//SONY PSX mini Controller (SCPH-1000R)
// Byte0 △ 0x01 / ○ 0x02 / × 0x04 / □ 0x08 / L2 0x10 / R2 0x20 / L1 0x40 / R1 0x80
// Byte1 HAT X方向 LEFT 0x00 - 0x04 - 0x08 RIGHT Y方向 UP 0x00 - 0x10 - 0x20 DOWN  SEL 0x01 START 0x02
//


// V1.1 Added
// --------------------------------------------------
//
#define VID2DC8PID2101		5				//8BitDo SN30 Pro
// 8BitDo SN30 Pro
//VID: 0x2DC8 PID: 0x2101 Connected.
//BUF: 01000008FF7FFF7F　←挿した後の初期値
//BUF: 01 00 00 00FF7FFF7F　←十字キー[上]を押した ★
//BUF: 01 00 00 04FF7FFF7F　←十字キー[下]を押した ★
//BUF: 01 00 00 06FF7FFF7F　←十字キー[左]を押した ★
//BUF: 01 00 00 02FF7FFF7F　←十字キー[右]を押した ★
//BUF: 01 01 00 08FF7FFF7F　←ボタン[A]を押した ★A(TRIGGER1)
//BUF: 01 02 00 08FF7FFF7F　←ボタン[B]を押した ★B(TRIGGER3)
//BUF: 01 08 00 08FF7FFF7F　←ボタン[X]を押した ◎B(TRIGGER3)
//BUF: 01 10 00 08FF7FFF7F　←ボタン[Y]を押した ◎A(TRIGGER1)
//BUF: 01 00 01 08FF7FFF7F　←ボタン[LT]を押した
//BUF: 01 00 02 08FF7FFF7F　←ボタン[RT]を押した
//BUF: 01 40 00 08FF7FFF7F　←ボタン[LB]を押した
//BUF: 01 80 00 08FF7FFF7F　←ボタン[RB]を押した
//BUF: 01 00 08 08FF7FFF7F　←ボタン[START]を押した ★START
//BUF: 01 00 04 08FF7FFF7F　←ボタン[SELECT]を押した ★SELECT
//BUF: 01 00 10 08FF7FFF7F　←ボタン[HOME]を押した
//
// Byte1: button [RB LB 0 Y    | X     0       B A]
// Byte2: button [0  0  0 HOME | START SELECT RT LT
// Byte3: HAT
// --------------------------------------------------


#define VID0CA3PID0027		6				//SEGA アストロシティミニ コントロールパッド
// SEGA アストロシティミニ コントロールパッド
// ※SEGA MDMINI系と同じ配置
//VID: 0xCA3 PID: 0x27 Connected.
//BUF: 01 7F 7F 7F 7F 0F 00 00　←挿した後の初期値
//BUF: 01 7F 7F 7F 00 0F 00 00　←十字キー[上]を押した ★
//BUF: 01 7F 7F 7F FF 0F 00 00　←十字キー[下]を押した ★
//BUF: 01 7F 7F 00 7F 0F 00 00　←十字キー[左]を押した ★
//BUF: 01 7F 7F FF 7F 0F 00 00　←十字キー[右]を押した ★
//BUF: 01 7F 7F 7F 7F 4F 00 00　←ボタン[D]を押した ◎B(TRIGGER3)
//BUF: 01 7F 7F 7F 7F 2F 00 00　←ボタン[E]を押した ◎A(TRIGGER1)
//BUF: 01 7F 7F 7F 7F 0F 02 00　←ボタン[F]を押した ◎B(TRIGGER3)
//BUF: 01 7F 7F 7F 7F 8F 00 00　←ボタン[A]を押した ★A(TRIGGER1)
//BUF: 01 7F 7F 7F 7F 1F 00 00　←ボタン[B]を押した ★B(TRIGGER3)
//BUF: 01 7F 7F 7F 7F 0F 01 00　←ボタン[C]を押した ◎A(TRIGGER1)
//BUF: 01 7F 7F 7F 7F 0F 20 00　←ボタン[START]を押した ★START
//BUF: 01 7F 7F 7F 7F 0F 10 00　←ボタン[CREDIT]を押した ★SELECT
//
// Byte3 X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte4 Y方向 UP   0x00-0x7f-0xff DOWN
// Byte5 button [ A D E     B      | 1 1 1 1 ]
// Byte6 button [ 0 0 START CREDIT | 0 0 F C ]

#define VID0AE4PID0702		7				//TAITO イーグレットツーミニ コントロールパッド
#define VID0AE4PID0703		8				//TAITO イーグレットツーミニ コントロールパネル
// TAITO イーグレットツーミニ コントロールパッド(PID: 0x702)
// TAITO イーグレットツーミニ コントロールパネル(PID: 0x703)
//
//VID: 0xAE4 PID: 0x702 Connected.
//VID: 0xAE4 PID: 0x703 Connected.
//BUF: 00 00 7F 7F 00 00 00 00　←挿した後の初期値
//BUF: 00 00 7F 00 00 00 00 00　←十字キー[上]を押した ★
//BUF: 00 00 7F FF 00 00 00 00　←十字キー[下]を押した ★
//BUF: 00 00 00 7F 00 00 00 00　←十字キー[左]を押した ★
//BUF: 00 00 FF 7F 00 00 00 00　←十字キー[右]を押した ★
//BUF: 10 00 7F 7F 00 00 00 00　←ボタン[A]を押した ★A(TRIGGER1)
//BUF: 04 00 7F 7F 00 00 00 00　←ボタン[B]を押した ★B(TRIGGER3)
//BUF: 02 00 7F 7F 00 00 00 00　←ボタン[C]を押した ◎A(TRIGGER1)
//BUF: 00 01 7F 7F 00 00 00 00　←ボタン[D]を押した ◎B(TRIGGER3)
//BUF: 08 00 7F 7F 00 00 00 00　←ボタン[E]を押した ◎A(TRIGGER1)
//BUF: 01 00 7F 7F 00 00 00 00　←ボタン[F]を押した ◎B(TRIGGER3)
//BUF: 40 00 7F 7F 00 00 00 00　←ボタン[真ん中のピンク]を押した ★SELECT
//BUF: 80 00 7F 7F 00 00 00 00　←ボタン[真ん中の青]を押した ★START
//BUF: 00 02 7F 7F 00 00 00 00　←ボタン[真ん中の白]を押した
//
// Byte0 button [ BLUE PINK 0 A | E B C     F ]
// Byte1 button [ 0    0    0 0 | 0 0 WHITE D ]
// Byte2 X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte3 Y方向 UP   0x00-0x7f-0xff DOWN


#define VID2F24PID0039		9				//Matflash F300 Elite
// Matflash F300 Elite
//
//VID: 0x2F24 PID: 0x39 Connected.
//BUF: 00 00 0F 80 80 80 80 00　←挿した後の初期値
//BUF: 00 00 00 80 80 80 80 00　←十字キー[上]を押した ★
//BUF: 00 00 04 80 80 80 80 00　←十字キー[下]を押した ★
//BUF: 00 00 06 80 80 80 80 00　←十字キー[左]を押した ★
//BUF: 00 00 02 80 80 80 80 FF　←十字キー[右]を押した ★
//BUF: 02 00 0F 80 80 80 80 00　←ボタン[A]を押した ★A(TRIGGER1)
//BUF: 04 00 0F 80 80 80 80 00　←ボタン[B]を押した ★B(TRIGGER3)
//BUF: 80 00 0F 80 80 80 80 00　←ボタン[RT]を押した ◎A(TRIGGER1)
//BUF: 40 00 0F 80 80 80 80 00　←ボタン[LT]を押した
//BUF: 01 00 0F 80 80 80 80 00　←ボタン[X]を押した ◎B(TRIGGER3)
//BUF: 08 00 0F 80 80 80 80 00　←ボタン[Y]を押した ◎A(TRIGGER1)
//BUF: 20 00 0F 80 80 80 80 00　←ボタン[RB]を押した ◎B(TRIGGER3)
//BUF: 10 00 0F 80 80 80 80 00　←ボタン[LB]を押した
//BUF: 00 02 0F 80 80 80 80 00　←ボタン[START]を押した ★START
//BUF: 00 01 0F 80 80 80 80 00　←ボタン[SELECT]を押した ★SELECT
//BUF: 00 10 0F 80 80 80 80 00　←ボタン[HOME]を押した
//
// Byte0 button [RT LT RB LB   | Y B A     X      ]
// Byte1 button [0  0  0  HOME | 0 0 START SELECT ]
// Byte2 HAT


#define VID054CPID09CC		10				//PS4 DUALSHOCK 4 (CUH-ZCT2J)
#define VID0F0DPID00ED		11				//HORI FIGHTING STICK MINI
// PS4 DUALSHOCK 4 (CUH-ZCT2J)
//
//VID: 0x54C PID: 0x9CC Connected.
//BUF: 01 7F 7F 7F 7F 08 00 38　←挿した後の初期値
//
// Byte1 L Stick X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte2 L Stick Y方向 UP   0x00-0x7f-0xff DOWN
// Byte3 R Stick X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte4 R Stick Y方向 UP   0x00-0x7f-0xff DOWN
// Byte5 button [ △ ○ × □ | HAT[4bit] ]
// Byte6 button [ RSTICK LSTICK OPTIONS SHARE | R2 L2 R1 L1 ]

//#define VID_PID_UNKNOWN		128				//Unknown Direct Input

// Byte1 X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte2 Y方向 UP   0x00-0x7f-0xff DOWN
// Byte3 X方向 LEFT 0x00-0x7f-0xff RIGHT
// Byte4 Y方向 UP   0x00-0x7f-0xff DOWN
// Byte5 0x1F A / 0x2F B
// Byte6 0x20 Start / 0x10 Select

// Byte0 button [ BLUE PINK 0 A | E B C     F ]
// Byte1 button [ 0    0    0 0 | 0 0 WHITE D ]

// VID&PIDのテーブル
const  uint16_t  tVID[] = {0x0CA3, 0x0CA3, 0x0F0D, 0x0810, 0x054c, 0x2DC8, 0x0CA3, 0x0AE4, 0x0AE4, 0x2F24, 0x054C, 0x0F0D};
const  uint16_t  tPID[] = {0x0025, 0x0024, 0x0138, 0xE501, 0x0CDA, 0x2101, 0x0027, 0x0702, 0x0703, 0x0039, 0x09CC, 0x00ED};



bool flagConnected;

// M5Faces GamePad key Assign
// UP     bit0
// DOWN   bit1
// LEFT   bit2
// RIGHT  bit3
// A      bit4
// B      bit5
// Select bit6
// Start  bit7


#ifdef FACES_GAMEPAD_MODE
unsigned char keycode = 0xFF;
#else
unsigned char keycode = 0x00;
#endif

unsigned char tKeycode = 0xFF;

#include "le3dp_rptparser.h"

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// M5 Faces Game Padモードでのビット配置
#define M5Faces_Up		0xfe
#define M5Faces_Down	0xfd
#define M5Faces_Left	0xfb
#define M5Faces_Right	0xf7
#define M5Faces_A		0xef
#define M5Faces_B		0xdf
#define M5Faces_Select	0xbf
#define M5Faces_Start	0x7f

void MakeKeyData_Hat(unsigned char data){
// Hat Dataのデコード
	if (data == 1 ) tKeycode = tKeycode & (M5Faces_Up & M5Faces_Right);
	if (data == 2 ) tKeycode = tKeycode & M5Faces_Right;
	if (data == 3 ) tKeycode = tKeycode & (M5Faces_Down & M5Faces_Right);
	if (data == 4 ) tKeycode = tKeycode & M5Faces_Down;

	if (data == 5 ) tKeycode = tKeycode & (M5Faces_Down & M5Faces_Left);
	if (data == 6 ) tKeycode = tKeycode & M5Faces_Left;
	if (data == 7 ) tKeycode = tKeycode & (M5Faces_Up & M5Faces_Left);
	if (data == 0 ) tKeycode = tKeycode & M5Faces_Up;
}

void MakeKeyData_XY(unsigned char x_data,unsigned char y_data){
// Hat Dataのデコード
	if (x_data < 0x40) tKeycode = tKeycode & M5Faces_Left;
	if (x_data > 0xC0) tKeycode = tKeycode & M5Faces_Right;

	if (y_data < 0x40) tKeycode = tKeycode & M5Faces_Up;
	if (y_data > 0xC0) tKeycode = tKeycode & M5Faces_Down;
}

void MakeKeyData_HatPSX(unsigned char data){
// Hat Dataのデコード(PSX)
	if ((data & 0x0c) == 0x00) tKeycode = tKeycode & M5Faces_Left;
	if ((data & 0x0c) == 0x08) tKeycode = tKeycode & M5Faces_Right;

	if ((data & 0x30) == 0x00) tKeycode = tKeycode & M5Faces_Up;
	if ((data & 0x30) == 0x20) tKeycode = tKeycode & M5Faces_Down;

}

void MakeKeyData_AB(unsigned char data_a,unsigned char bit_a,unsigned char data_b,unsigned char bit_b){
// AB Dataのデコード
	if (Get_Bit(data_a,bit_a) != 0x00) tKeycode = tKeycode & M5Faces_A;
	if (Get_Bit(data_b,bit_b) != 0x00) tKeycode = tKeycode & M5Faces_B;
}

void MakeKeyData_SelStart(unsigned char data_sel,unsigned char bit_sel,unsigned char data_start,unsigned char bit_start){
// Select Start Dataのデコード
	if (Get_Bit(data_sel,bit_sel) != 0x00) tKeycode = tKeycode & M5Faces_Select;
	if (Get_Bit(data_start,bit_start) != 0x00) tKeycode = tKeycode & M5Faces_Start;
}


void requestEvent()
{
	Wire.write(keycode);
}

USB Usb;
USBHub Hub(&Usb);
HIDUniversal Hid(&Usb);
JoystickEvents JoyEvents;
JoystickReportParser Joy(&JoyEvents);



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
	flagConnected = false;

//I2C有効、割り込み使用
	Wire.begin(FACES_KEYBOARD_I2C_ADDR);
	Wire.onRequest(requestEvent);

//UART一応有効化(ファーム書き換え時に困るため）
	Serial.begin( 115200 );

	#ifdef DEBUG_PRINT 
	#if !defined(__MIPSEL__)
//  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
	#endif
	Serial.println("Start");
	#endif

	if (Usb.Init() == -1){
		#ifdef DEBUG_PRINT 
		Serial.println("OSC did not start.");
		#endif
	}
	delay( 200 );

	if (!Hid.SetReportParser(0, &Joy))
	ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );
}

void loop()
{
// ループ処理

	uint8_t rcode = 0;
	byte num_conf = 0;
	USB_DEVICE_DESCRIPTOR buf;
	int i;


	Usb.Task();
	if( Usb.getUsbTaskState() == USB_STATE_RUNNING ) {
// USBのINTERVALアクセスが始まると以下のルーチンが実行される

//コントローラの１回目の接続時のみ対応コントローラかどうかのチェックを行う。
//対応しているコントローラの場合対応した番号を割り付ける
		if (flagConnected == false) {
			flagConnected = true;
			rcode = Usb.getDevDescr(1, 0, 0x12, ( uint8_t *)&buf );
			if( rcode ) {
				//普通はここには来ない、
				//USB Device Descriptorが取得出来なかった場合など
				#ifdef DEBUG_PRINT 
				Serial.print("USB ERROR: "); Serial.println( rcode );
				#endif
				while (1);
			}
			#ifdef DEBUG_PRINT 
			Serial.print("VID: 0x"); Serial.print(buf.idVendor, HEX);
			Serial.print(" PID: 0x"); Serial.print(buf.idProduct, HEX);
			Serial.println (" Connected.");
			#endif

//
			JoyCl = 0xFF;
			//対応コントローラチェック
			//非対応コントローラはVIDとPIDをテーブルに追加して、
			//RAW Dataのデコードを追記すれば対応可能
			for (i = 0 ;i<JoyClMax+1;i++){
				if ((buf.idVendor == tVID[i]) && (buf.idProduct == tPID[i])) {
					JoyCl = i;
					#ifdef DEBUG_PRINT 
					Serial.println("Support USB JoyStick");
					Serial.print("ID: ");
					Serial.println(JoyCl);
					#endif
				}
			}

		}
	}else{
		flagConnected = false;
	}

// Direction button
}

void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt)
{
// USBのINTERVALデータが一つ前と異なる場合、変化がある場合ここがCallされる。

	//RAW DATA出力（デバッグ用）
	#ifdef USB_RAW_DISPLAY
	Serial.print("BUF: ");
	PrintHex<uint8_t>(evt->raw_data[0], 0x0);
	PrintHex<uint8_t>(evt->raw_data[1], 0x0);
	PrintHex<uint8_t>(evt->raw_data[2], 0x0);
	PrintHex<uint8_t>(evt->raw_data[3], 0x0);
	PrintHex<uint8_t>(evt->raw_data[4], 0x0);
	PrintHex<uint8_t>(evt->raw_data[5], 0x0);
	PrintHex<uint8_t>(evt->raw_data[6], 0x0);
	PrintHex<uint8_t>(evt->raw_data[7], 0x0);
	Serial.println("");
	#endif
	
	tKeycode = 0xff;			//PADデータ初期値
	switch(JoyCl)
	{
		case VID0CA3PID0025: 	//MD PAD USB (MK-16500)
		MakeKeyData_XY(evt->raw_data[3],evt->raw_data[4]);
		MakeKeyData_AB(evt->raw_data[5],6,evt->raw_data[5],5);			// A:A B:B
		MakeKeyData_SelStart(evt->raw_data[6],1,evt->raw_data[6],5);	// SEL:C START:START
		break;

		case VID0CA3PID0024: 	//MD 6B PAD USB (HAA-2522)
		MakeKeyData_XY(evt->raw_data[3],evt->raw_data[4]);
		MakeKeyData_AB(evt->raw_data[5],6,evt->raw_data[5],5);			// A:A B:B
		MakeKeyData_SelStart(evt->raw_data[6],4,evt->raw_data[6],5);	// SEL:SELECT START:START
		break;

		case VID0F0DPID0138: 	//KONAMI PCEmini Controller(HT-002)
		MakeKeyData_Hat(evt->raw_data[2]);
		MakeKeyData_AB(evt->raw_data[0],2,evt->raw_data[0],1);			// A:I B:II
		MakeKeyData_SelStart(evt->raw_data[1],0,evt->raw_data[6],1);	// SEL:SELECT START:START
		break;

		case VID0810PIDE501: 	//NB NES USB Controller
		MakeKeyData_XY(evt->raw_data[3],evt->raw_data[4]);
		MakeKeyData_AB(evt->raw_data[5],5,evt->raw_data[5],4);			// A:A B:B
		MakeKeyData_SelStart(evt->raw_data[6],4,evt->raw_data[6],5);	// SEL:SELECT START:START
		break;

		case VID054CPID0CDA: 	//SONY PSX mini Controller (SCPH-1000R)
		MakeKeyData_HatPSX(evt->raw_data[1]);
		MakeKeyData_AB(evt->raw_data[0],1,evt->raw_data[0],2);				// A:× B:○
		MakeKeyData_SelStart(evt->raw_data[1],0,evt->raw_data[1],1);		// SEL:SELECT START:START
		break;

		case VID2DC8PID2101:	//8BitDo SN30 Pro
		MakeKeyData_Hat(evt->raw_data[3]);
		MakeKeyData_AB(evt->raw_data[1],4,evt->raw_data[1],3);				// A:Y B:X
		MakeKeyData_AB(evt->raw_data[1],0,evt->raw_data[1],1);				// A:A B:B
		MakeKeyData_SelStart(evt->raw_data[2],2,evt->raw_data[2],3);		// SEL:SELECT START:START
		break;

		case VID0CA3PID0027:	//SEGA アストロシティミニ コントロールパッド
		MakeKeyData_XY(evt->raw_data[3],evt->raw_data[4]);
		MakeKeyData_AB(evt->raw_data[5],5,evt->raw_data[5],6);			// A:E B:D
		MakeKeyData_AB(evt->raw_data[6],0,evt->raw_data[6],1);			// A:C B:F
		MakeKeyData_AB(evt->raw_data[5],7,evt->raw_data[5],4);			// A:A B:B
		MakeKeyData_SelStart(evt->raw_data[6],4,evt->raw_data[6],5);	// SEL:CREDIT START:START
		break;

		case VID0AE4PID0702:	//TAITO イーグレットツーミニ コントロールパッド
		case VID0AE4PID0703:	//TAITO イーグレットツーミニ コントロールパネル
		MakeKeyData_XY(evt->raw_data[2],evt->raw_data[3]);
		MakeKeyData_AB(evt->raw_data[0],4,evt->raw_data[0],2);			// A:A B:B
		MakeKeyData_AB(evt->raw_data[0],1,evt->raw_data[1],0);			// A:C B:D
		MakeKeyData_AB(evt->raw_data[0],3,evt->raw_data[0],0);			// A:E B:F
		MakeKeyData_SelStart(evt->raw_data[0],6,evt->raw_data[0],7);	// SEL:PINK START:BLUE
		break;

		case VID2F24PID0039:	//Matflash F300 Elite
		MakeKeyData_Hat(evt->raw_data[2]);
		MakeKeyData_AB(evt->raw_data[0],1,evt->raw_data[0],2);				// A:A B:B
		MakeKeyData_AB(evt->raw_data[0],7,evt->raw_data[0],5);				// A:RT B:RB
		MakeKeyData_AB(evt->raw_data[0],3,evt->raw_data[0],0);				// A:Y B:X
		MakeKeyData_SelStart(evt->raw_data[1],0,evt->raw_data[1],1);		// SEL:SELECT START:START
		break;

		case VID054CPID09CC:	//PS4 DUALSHOCK 4 (CUH-ZCT2J)
		case VID0F0DPID00ED:	//HORI FIGHTING STICK MINI
		MakeKeyData_Hat((evt->raw_data[5]) & 0x0f);
		MakeKeyData_AB(evt->raw_data[5],5,evt->raw_data[5],6);				// A:× B:○
		MakeKeyData_SelStart(evt->raw_data[6],4,evt->raw_data[6],5);		// SEL:SHARE START:OPTION
		break;

		default:				//不明コントローラ
		MakeKeyData_XY(evt->raw_data[0],evt->raw_data[1]);
		MakeKeyData_AB(evt->raw_data[2],1,evt->raw_data[2],0);				// A:A B:B
		MakeKeyData_SelStart(evt->raw_data[2],2,evt->raw_data[2],3);		// SEL:C START:START
		break;

	}

	if (tKeycode != 0xff) IRQ_0;		//キー入力あり
	else IRQ_1;

	keycode = 	tKeycode;				//I2C Dataへ転送
}
