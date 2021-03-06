//
// Created by 周立海 on 2017/4/24.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "PublicFunction.h"
#include "ctrlBase.h"

const INT8U auchCRCHi[] = { 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40 };

const INT8U auchCRCLo[] = { 0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2,
		0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
		0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
		0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F,
		0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6,
		0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1,
		0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
		0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB,
		0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA,
		0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5,
		0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
		0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
		0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE,
		0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79,
		0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
		0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73,
		0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
		0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D,
		0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
		0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F,
		0x8D, 0x4D, 0x4C, 0x8C, 0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86,
		0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40 };

INT16U Get_Crc16(INT8U* puchMsg, INT16U usDataLen) {
	INT8U uchCRCHi = 0xFF;
	INT8U uchCRCLo = 0xFF;
	INT8U uIndex;
	while (usDataLen--)
	{
		uIndex = uchCRCHi ^ *puchMsg++;
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex];
		uchCRCLo = auchCRCLo[uIndex];
	}
	return (uchCRCHi << 8 | uchCRCLo);
}

int CtrlModelIsPlugIn() {
	if (1 == gpio_readbyte("/dev/gpiZB_STATE")) {
		return 1;
	} else {
		return 0;
	}
}

int CtrlInitCommPort() {
	int serial_fd = OpenCom(5, 19200, (unsigned char *) "even", 1, 8);
	if (serial_fd <= 0) {
		return -1;
	} else {
		return serial_fd;
	}
}

void CtrlInitModelState() {
	gpio_writebyte("/dev/gpoZAIBO_RST", 0);
	gpio_writebyte("/dev/gpoZAIBO_RST", 1);
	gpio_writebyte("/dev/gpoCTL_EN", 1);

	usleep(80 * 1000); //等待至少60ms
	return;
}

INT8U ReadCmd(int fd, INT8U* rdata1, INT8U* rdata2) {
	INT8U readBuf[CTRL_LEN];
	memset(readBuf, 0, CTRL_LEN);

	int n = (int) read(fd, readBuf, CTRL_LEN);
	if (n < 0) {
		return 0xFF;
	}

	int start = 0;
	for (int i = 0; i < n; i++) {
		if (readBuf[i] == 0x68)
			start = i;
	}

	if ((readBuf[start + 4] << 8) + readBuf[start + 5] != Get_Crc16(&readBuf[start], 4)) {
		return 0xFF;
	}

	*rdata1 = readBuf[start + 2];
	*rdata2 = readBuf[start + 3];
	return readBuf[start + 1];
}

INT8U SendCmd(int fd, INT8U cmd, INT8U getcmd, INT8U* data1, INT8U* data2) {
	INT8U sendbuf[CTRL_LEN];
	memset(sendbuf, 0x00, CTRL_LEN);

	sendbuf[0] = CTRL_HEAD;
	sendbuf[1] = cmd;
	sendbuf[2] = *data1;
	sendbuf[3] = *data2;
	INT16U crcdata = Get_Crc16(sendbuf, 4);
	sendbuf[4] = (INT8U) ((crcdata & 0xff00) >> 8);
	sendbuf[5] = (INT8U) (crcdata & 0x00ff);
	sendbuf[6] = CTRL_END;

	int n = (int) write(fd, sendbuf, CTRL_LEN);
	if (n != CTRL_LEN) {
		return 0xFF;
	}

	usleep(200 * 1000);
	return ReadCmd(fd, data1, data2);
}

void BuildByte_by_state(INT8U* abyte, INT8U state, INT8U place) {
	*abyte = (INT8U) ((state == 0x01) ? *abyte | (0x01 << place) : *abyte & ~(0x01 << place));
}

INT8U SetCtrl_CMD(int serial_fd, INT8U lun1_state, INT8U lun1_red,
		INT8U lun1_green, INT8U lun2_state, INT8U lun2_red, INT8U lun2_green,
		INT8U gongk_led, INT8U diank_led, INT8U alm_state, INT8U baodian_led) {

	INT8U data_low = 0;
	INT8U data_hig = 0;

	BuildByte_by_state(&data_low, lun1_state, 0); //b0	轮次1	分/合闸
	BuildByte_by_state(&data_low, lun1_red, 1); //b1	轮次1	分-红
	BuildByte_by_state(&data_low, lun1_green, 2); //b2	轮次1	合-绿
	BuildByte_by_state(&data_low, lun2_state, 3); //b3	轮次2	分/合闸
	BuildByte_by_state(&data_low, lun2_red, 4); //b4	轮次2	分-红
	BuildByte_by_state(&data_low, lun2_green, 5); //b5	轮次2	合-绿
	BuildByte_by_state(&data_low, gongk_led, 6); //b6	功控指示灯
	BuildByte_by_state(&data_low, diank_led, 7); //b7	电控指示灯
	BuildByte_by_state(&data_hig, alm_state, 0); //B0	告警继电器合闸
	BuildByte_by_state(&data_hig, baodian_led, 1); //B1	保电指示灯

	CtrlModelIsPlugIn();
	CtrlInitModelState();
	int fd = CtrlInitCommPort();

	INT8U res = SendCmd(fd, SEND_CTRL_CMD, RECE_CTRL_CMD, &data_low,
						&data_hig);
	close(fd);
	return res;
}
