/*
	MSX KEYMATRIX DUMP program
	Copyright 2023 @V9938
	
	V1.0		1st version
*/

#include <stdio.h>
#include<string.h>
#include <msx.h>
#include <sys/ioctl.h>

#define VERSION "V1.0"
#define DATE "2023/03"



//Assembler SubRoutine
void setSlot(){
#asm
	ld a,(0fcc1h)		;EXPTBL 
    ld (biosSlot),a		;MSXDOSからBIOS Callする為にMAINROMスロットを事前セットしておく
	ret
#endasm
}

void clearKeybuf() __z88dk_fastcall __naked {
// BDOS Call 06 の呼び出し

#asm
	di
	ld a,(0f3f8h)		;PUTPNT
	ld (0f3fah),a		;GETPNT

	ld a,(0f3f9h)		;PUTPNT+1
	ld (0f3fbh),a		;GETPNT+1

	ei
	ret
	
#endasm
}

unsigned char getKeyMatrix(unsigned char read_matrix) __z88dk_fastcall __naked {
// カーソルとジョイスティックの現在の情報を入手します。
#asm
	di
	ld a,l				;KEYBOARD matrix
	call callSnsmat		;BIOS SNSMAT
	ld l,a
	ei
	ret
	
callSnsmat:
	rst 030h
biosSlot:
	db		00
	dw		0141h		;SNSMAT 
	ret
	
#endasm
}
void key_decodeJPN(unsigned char matrix,unsigned char keydata){
	unsigned char keydataR;

	keydataR = keydata ^ 0xff;
	switch (matrix){
		case 0:
			if (keydataR & (1<<0)) printf("[0] ");
			if (keydataR & (1<<1)) printf("[1!] ");
			if (keydataR & (1<<2)) printf("[2\"] ");
			if (keydataR & (1<<3)) printf("[3#] ");
			if (keydataR & (1<<4)) printf("[4$] ");
			if (keydataR & (1<<5)) printf("[5%%] ");
			if (keydataR & (1<<6)) printf("[6&] ");
			if (keydataR & (1<<7)) printf("[7\'] ");
		break;
		case 1:
			if (keydataR & (1<<0)) printf("[8(] ");
			if (keydataR & (1<<1)) printf("[9)] ");
			if (keydataR & (1<<2)) printf("[-=] ");
			if (keydataR & (1<<3)) printf("[^~] ");
			if (keydataR & (1<<4)) printf("[\\|] ");
			if (keydataR & (1<<5)) printf("[@`] ");
			if (keydataR & (1<<6)) printf("[[{] ");
			if (keydataR & (1<<7)) printf("[;+] ");
		break;

		case 2:
			if (keydataR & (1<<0)) printf("[:*] ");
			if (keydataR & (1<<1)) printf("[]}] ");
			if (keydataR & (1<<2)) printf("[,<] ");
			if (keydataR & (1<<3)) printf("[.>] ");
			if (keydataR & (1<<4)) printf("[/?] ");
			if (keydataR & (1<<5)) printf("[_] ");
			if (keydataR & (1<<6)) printf("[aA] ");
			if (keydataR & (1<<7)) printf("[bB] ");
		break;

		case 3:
			if (keydataR & (1<<0)) printf("[cC] ");
			if (keydataR & (1<<1)) printf("[dD] ");
			if (keydataR & (1<<2)) printf("[eE] ");
			if (keydataR & (1<<3)) printf("[fF] ");
			if (keydataR & (1<<4)) printf("[gG] ");
			if (keydataR & (1<<5)) printf("[hH] ");
			if (keydataR & (1<<6)) printf("[iI] ");
			if (keydataR & (1<<7)) printf("[jJ] ");
		break;

		case 4:
			if (keydataR & (1<<0)) printf("[kK] ");
			if (keydataR & (1<<1)) printf("[lL] ");
			if (keydataR & (1<<2)) printf("[mM] ");
			if (keydataR & (1<<3)) printf("[nN] ");
			if (keydataR & (1<<4)) printf("[oO] ");
			if (keydataR & (1<<5)) printf("[pP] ");
			if (keydataR & (1<<6)) printf("[qQ] ");
			if (keydataR & (1<<7)) printf("[rR] ");
		break;

		case 5:
			if (keydataR & (1<<0)) printf("[sS] ");
			if (keydataR & (1<<1)) printf("[tT] ");
			if (keydataR & (1<<2)) printf("[uU] ");
			if (keydataR & (1<<3)) printf("[vV] ");
			if (keydataR & (1<<4)) printf("[wW] ");
			if (keydataR & (1<<5)) printf("[xX] ");
			if (keydataR & (1<<6)) printf("[yY] ");
			if (keydataR & (1<<7)) printf("[zZ] ");
		break;

		case 6:
			if (keydataR & (1<<0)) printf("[SHIFT] ");
			if (keydataR & (1<<1)) printf("[CTRL] ");
			if (keydataR & (1<<2)) printf("[GRAPH] ");
			if (keydataR & (1<<3)) printf("[CAPSLOCK] ");
			if (keydataR & (1<<4)) printf("[KANA] ");
			if (keydataR & (1<<5)) printf("[F1/F6] ");
			if (keydataR & (1<<6)) printf("[F2/F7] ");
			if (keydataR & (1<<7)) printf("[F3/F8] ");
		break;

		case 7:
			if (keydataR & (1<<0)) printf("[F4/F9] ");
			if (keydataR & (1<<1)) printf("[F5/F10] ");
			if (keydataR & (1<<2)) printf("[ESC] ");
			if (keydataR & (1<<3)) printf("[TAB] ");
			if (keydataR & (1<<4)) printf("[STOP] ");
			if (keydataR & (1<<5)) printf("[BS] ");
			if (keydataR & (1<<6)) printf("[SELECT] ");
			if (keydataR & (1<<7)) printf("[RETURN] ");
		break;

		case 8:
			if (keydataR & (1<<0)) printf("[SPACE] ");
			if (keydataR & (1<<1)) printf("[CLS/HOME] ");
			if (keydataR & (1<<2)) printf("[INS] ");
			if (keydataR & (1<<3)) printf("[DEL] ");
			if (keydataR & (1<<4)) printf("[LEFT] ");
			if (keydataR & (1<<5)) printf("[UP] ");
			if (keydataR & (1<<6)) printf("[DOWN] ");
			if (keydataR & (1<<7)) printf("[RIGHT] ");
		break;

		case 9:
			if (keydataR & (1<<0)) printf("[OPT0] ");
			if (keydataR & (1<<1)) printf("[OPT1] ");
			if (keydataR & (1<<2)) printf("[OPT2] ");
			if (keydataR & (1<<3)) printf("[TEN 0] ");
			if (keydataR & (1<<4)) printf("[TEN 1] ");
			if (keydataR & (1<<5)) printf("[TEN 2] ");
			if (keydataR & (1<<6)) printf("[TEN 3] ");
			if (keydataR & (1<<7)) printf("[TEN 4] ");
		break;

		case 10:
			if (keydataR & (1<<0)) printf("[TEN 5] ");
			if (keydataR & (1<<1)) printf("[TEN 6] ");
			if (keydataR & (1<<2)) printf("[TEN 7] ");
			if (keydataR & (1<<3)) printf("[TEN 8] ");
			if (keydataR & (1<<4)) printf("[TEN 9] ");
			if (keydataR & (1<<5)) printf("[TEN -] ");
			if (keydataR & (1<<6)) printf("[TEN ,] ");
			if (keydataR & (1<<7)) printf("[TEN .] ");
		break;

	}
	

}


void wait(int waitnum){
	unsigned char tmp;
	unsigned char *jiffy;
	
	jiffy = (unsigned char *) 0xfc9e;				//JIFFY
	tmp = *jiffy;
	tmp = tmp + (unsigned char) waitnum;
	if (*jiffy > tmp)
		while (*jiffy < 0x80);
		
	while (*jiffy < tmp);

}
int main(int argc,char *argv[])
{

	unsigned char matrix;
	unsigned char keydata[11];
	unsigned char old_keydata[11];
	unsigned char space_num;
	unsigned char keychg;
	
	
	unsigned char databuf[4];
	
	printf("MSX KEYDUMP %s\n",VERSION);
	printf("Copyright %s @v9938\n\n",DATE);
	printf("\n Program end [SPACE]x3\n");

	setSlot();

	space_num = 0;
	for (matrix = 0 ; matrix < 11 ; matrix++){
		old_keydata[matrix] = getKeyMatrix(matrix);
	}

	while (1){
		
		keychg = 0;
		for (matrix = 0 ; matrix < 11 ; matrix++){
			
			keydata[matrix] = getKeyMatrix(matrix);
			if (keydata[matrix] != old_keydata[matrix]){
				
				printf("[M%02d]%02X: ",matrix,keydata[matrix]);
				if (keydata[matrix]>old_keydata[matrix]){
					printf("UP:");
					key_decodeJPN(matrix,old_keydata[matrix]);
				}else{
					printf("DN:");
					key_decodeJPN(matrix,keydata[matrix]);
				}
				
				if ((matrix == 8) && ((keydata[matrix]== 0xff) | (keydata[matrix]== 0xfe))) space_num++;
				else space_num = 0;

				old_keydata[matrix] = keydata[matrix];
				keychg = 1;
				
			}
			if ((matrix == 10) & (keychg == 1))	{
				printf("\n");
				keychg = 0;
			}
		}
		if (space_num == 6) {
			printf("\n\nDone. Thank you using!\n");
			clearKeybuf();
			return 0;
		}
	}

	

}
