/*
 * readplc.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef READPLC_H_
#define READPLC_H_
#include "StdDataType.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "format3762.h"
#include "lib3762.h"

pthread_attr_t readplc_attr_t;
int thread_readplc_id;        //载波（I型）
pthread_t thread_readplc;
extern void readplc_proccess();
extern INT16S getTaskIndex(INT8U port);

#define NONE_PROCE	 		-1
#define DATE_CHANGE   		0
#define DATA_REAL     		1
#define METER_SEARCH  		2
#define TASK_PROCESS  		3
#define SLAVE_COMP    		4
#define INIT_MASTERADDR    	5
#define AUTO_REPORT    		6

#define ALLOK 2
#define ZBBUFSIZE 512
#define BUFSIZE645 256
#define TASK6012_MAX 256
#define FANGAN_ITEM_MAX 64
#define DLT645_07  2
#define DLT698  3
#define CJT188  4
#define NUM_07DI_698OAD 100
//---------------------------------------------------------------
typedef struct
{
	INT8U index;
	INT8U dataFlag[4];
	char name[40];
}AutoEventInfo_Meter;//电能表主动上报事件信息：状态字和数据标识的对应关系
static AutoEventInfo_Meter autoEventInfo_meter[] =
{
		{	20	,{	0x19,0x01,0x00,0x01	},	"A 相过流次数	"	}	,
		{	20	,{	0x19,0x01,0x01,0x01	},	"A 相过流	"	}	,
		{	20	,{	0x19,0x01,0x21,0x01	},	"A 相过流	"	}	,
		{	36	,{	0x19,0x02,0x00,0x01	},	"B 相过流次数	"	}	,
		{	36	,{	0x19,0x02,0x01,0x01	},	"B 相过流	"	}	,
		{	36	,{	0x19,0x02,0x21,0x01	},	"B 相过流	"	}	,
		{	52	,{	0x19,0x03,0x00,0x01	},	"C 相过流	次数"	}	,
		{	52	,{	0x19,0x03,0x01,0x01	},	"C 相过流	"	}	,
		{	52	,{	0x19,0x03,0x21,0x01	},	"C 相过流	"	}	,
		{	69	,{	0x03,0x11,0x00,0x00	},	"电能表掉电事件次数"	}	,
		{	69	,{	0x03,0x11,0x00,0x01	},	"电能表掉电事件"	}	,
		{	14	,{	0x1d,0x00,0x00,0x01	},	"跳闸次数次数	"	}	,
		{	14	,{	0x1d,0x00,0x01,0x01	},	"跳闸次数	"	}	,
		{	15	,{	0x1e,0x00,0x00,0x01	},	"合闸次数	次数"	}	,
		{	15	,{	0x1e,0x00,0x01,0x01	},	"合闸次数	"	}	,
		{	16	,{	0x10,0x01,0x00,0x01	},	"A 相失压次数	"	}	,
		{	16	,{	0x10,0x01,0x01,0x01	},	"A 相失压	"	}	,
		{	16	,{	0x10,0x01,0x25,0x01	},	"A 相失压	"	}	,
		{	32	,{	0x10,0x02,0x00,0x01	},	"B 相失压次数	"	}	,
		{	32	,{	0x10,0x02,0x01,0x01	},	"B 相失压	"	}	,
		{	32	,{	0x10,0x02,0x25,0x01	},	"B 相失压	"	}	,
		{	48	,{	0x10,0x03,0x00,0x01	},	"C 相失压次数	"	}	,
		{	48	,{	0x10,0x03,0x01,0x01	},	"C 相失压	"	}	,
		{	48	,{	0x10,0x03,0x25,0x01	},	"C 相失压	"	}	,
		{	17	,{	0x11,0x01,0x00,0x01	},	"A 相欠压次数	"	}	,
		{	17	,{	0x11,0x01,0x01,0x01	},	"A 相欠压	"	}	,
		{	17	,{	0x11,0x01,0x25,0x01	},	"A 相欠压	"	}	,
		{	33	,{	0x11,0x02,0x00,0x01	},	"B 相欠压次数	"	}	,
		{	33	,{	0x11,0x02,0x01,0x01	},	"B 相欠压	"	}	,
		{	33	,{	0x11,0x02,0x25,0x01	},	"B 相欠压	"	}	,
		{	49	,{	0x11,0x03,0x00,0x01	},	"C 相欠压	次数"	}	,
		{	49	,{	0x11,0x03,0x01,0x01	},	"C 相欠压	"	}	,
		{	49	,{	0x11,0x03,0x25,0x01	},	"C 相欠压	"	}	,
		{	18	,{	0x12,0x01,0x00,0x01	},	"A 相过压	次数"	}	,
		{	18	,{	0x12,0x01,0x01,0x01	},	"A 相过压	"	}	,
		{	18	,{	0x12,0x01,0x25,0x01	},	"A 相过压	"	}	,
		{	34	,{	0x12,0x02,0x00,0x01	},	"B 相过压次数	"	}	,
		{	34	,{	0x12,0x02,0x01,0x01	},	"B 相过压	"	}	,
		{	34	,{	0x12,0x02,0x25,0x01	},	"B 相过压	"	}	,
		{	50	,{	0x12,0x03,0x00,0x01	},	"C 相过压次数	"	}	,
		{	50	,{	0x12,0x03,0x01,0x01	},	"C 相过压	"	}	,
		{	50	,{	0x12,0x03,0x25,0x01	},	"C 相过压	"	}	,
		{	19	,{	0x18,0x01,0x00,0x01	},	"A 相失流次数	"	}	,
		{	19	,{	0x18,0x01,0x01,0x01	},	"A 相失流	"	}	,
		{	19	,{	0x18,0x01,0x21,0x01	},	"A 相失流	"	}	,
		{	35	,{	0x18,0x02,0x00,0x01	},	"B 相失流次数	"	}	,
		{	35	,{	0x18,0x02,0x01,0x01	},	"B 相失流	"	}	,
		{	35	,{	0x18,0x02,0x21,0x01	},	"B 相失流	"	}	,
		{	51	,{	0x18,0x03,0x00,0x01	},	"C 相失流	次数"	}	,
		{	51	,{	0x18,0x03,0x01,0x01	},	"C 相失流	"	}	,
		{	51	,{	0x18,0x03,0x21,0x01	},	"C 相失流	"	}	,
		{	21	,{	0x1c,0x01,0x00,0x01	},	"A 相过载次数	"	}	,
		{	21	,{	0x1c,0x01,0x01,0x01	},	"A 相过载	"	}	,
		{	21	,{	0x1c,0x01,0x12,0x01	},	"A 相过载	"	}	,
		{	37	,{	0x1c,0x02,0x00,0x01	},	"B 相过载次数	"	}	,
		{	37	,{	0x1c,0x02,0x01,0x01	},	"B 相过载	"	}	,
		{	37	,{	0x1c,0x02,0x12,0x01	},	"B 相过载	"	}	,
		{	53	,{	0x1c,0x03,0x00,0x01	},	"C 相过载	次数"	}	,
		{	53	,{	0x1c,0x03,0x01,0x01	},	"C 相过载	"	}	,
		{	53	,{	0x1c,0x03,0x12,0x01	},	"C 相过载	"	}	,
		{	23	,{	0x13,0x01,0x00,0x01	},	"A 相断相次数	"	}	,
		{	23	,{	0x13,0x01,0x01,0x01	},	"A 相断相	"	}	,
		{	23	,{	0x13,0x01,0x25,0x01	},	"A 相断相	"	}	,
		{	39	,{	0x13,0x02,0x00,0x01	},	"B 相断相次数	"	}	,
		{	39	,{	0x13,0x02,0x01,0x01	},	"B 相断相	"	}	,
		{	39	,{	0x13,0x02,0x25,0x01	},	"B 相断相	"	}	,
		{	55	,{	0x13,0x03,0x00,0x01	},	"C 相断相	次数"	}	,
		{	55	,{	0x13,0x03,0x01,0x01	},	"C 相断相	"	}	,
		{	55	,{	0x13,0x03,0x25,0x01	},	"C 相断相	"	}	,
		{	24	,{	0x1a,0x01,0x00,0x01	},	"A 相断流	次数"	}	,
		{	24	,{	0x1a,0x01,0x01,0x01	},	"A 相断流	"	}	,
		{	24	,{	0x1a,0x01,0x21,0x01	},	"A 相断流	"	}	,
		{	40	,{	0x1a,0x02,0x00,0x01	},	"B 相断流次数	"	}	,
		{	40	,{	0x1a,0x02,0x01,0x01	},	"B 相断流	"	}	,
		{	40	,{	0x1a,0x02,0x21,0x01	},	"B 相断流	"	}	,
		{	56	,{	0x1a,0x03,0x00,0x01	},	"C 相断流次数	"	}	,
		{	56	,{	0x1a,0x03,0x01,0x01	},	"C 相断流	"	}	,
		{	56	,{	0x1a,0x03,0x21,0x01	},	"C 相断流	"	}	,
		{	22	,{	0x1b,0x01,0x00,0x01	},	"A 相功率反向次数	"	}	,
		{	22	,{	0x1b,0x01,0x01,0x01	},	"A 相功率反向	"	}	,
		{	22	,{	0x1b,0x01,0x12,0x01	},	"A 相功率反向	"	}	,
		{	38	,{	0x1b,0x02,0x00,0x01	},	"B 相功率反向次数	"	}	,
		{	38	,{	0x1b,0x02,0x01,0x01	},	"B 相功率反向	"	}	,
		{	38	,{	0x1b,0x02,0x12,0x01	},	"B 相功率反向	"	}	,
		{	54	,{	0x1b,0x03,0x00,0x01	},	"C 相功率反向次数	"	}	,
		{	54	,{	0x1b,0x03,0x01,0x01	},	"C 相功率反向	"	}	,
		{	54	,{	0x1b,0x03,0x12,0x01	},	"C 相功率反向	"	}	,
		{	64	,{	0x14,0x00,0x00,0x01	},	"电压逆相序次数"	}	,
		{	64	,{	0x14,0x00,0x01,0x01	},	"电压逆相序"	}	,
		{	64	,{	0x14,0x00,0x12,0x01	},	"电压逆相序"	}	,
		{	65	,{	0x15,0x00,0x00,0x01	},	"电流逆相序次数"	}	,
		{	65	,{	0x15,0x00,0x01,0x01	},	"电流逆相序"	}	,
		{	65	,{	0x15,0x00,0x12,0x01	},	"电流逆相序"	}	,
		{	66	,{	0x16,0x00,0x00,0x01	},	"电压不平衡次数"	}	,
		{	66	,{	0x16,0x00,0x01,0x01	},	"电压不平衡"	}	,
		{	66	,{	0x16,0x00,0x13,0x01	},	"电压不平衡"	}	,
		{	67	,{	0x17,0x00,0x00,0x01	},	"电流不平衡次数"	}	,
		{	67	,{	0x17,0x00,0x01,0x01	},	"电流不平衡"	}	,
		{	67	,{	0x17,0x00,0x13,0x01	},	"电流不平衡"	}	,
		{	71	,{	0x1f,0x00,0x00,0x01	},	"功率因数超下限次数"	}	,
		{	71	,{	0x1f,0x00,0x01,0x01	},	"功率因数超下限"	}	,
		{	71	,{	0x1f,0x00,0x06,0x01	},	"功率因数超下限"	}	,
		{	74	,{	0x03,0x05,0x00,0x00	},	"全失压次数	"	}	,
		{	74	,{	0x03,0x05,0x00,0x01	},	"全失压	"	}	,
		{	80	,{	0x03,0x30,0x00,0x00	},	"编程事件次数	"	}	,
		{	80	,{	0x03,0x30,0x00,0x01	},	"编程事件	"	}	,
		{	81	,{	0x03,0x30,0x01,0x00	},	"电能表清零事件次数"	}	,
		{	81	,{	0x03,0x30,0x01,0x01	},	"电能表清零事件"	}	,
		{	82	,{	0x03,0x30,0x02,0x00	},	"电能表需量清零次数"	}	,
		{	82	,{	0x03,0x30,0x02,0x01	},	"电能表需量清零"	}	,
		{	83	,{	0x03,0x30,0x03,0x00	},	"电能表事件清零次数"	}	,
		{	83	,{	0x03,0x30,0x03,0x01	},	"电能表事件清零"	}	,
		{	84	,{	0x03,0x30,0x04,0x00	},	"校时事件次数	"	}	,
		{	84	,{	0x03,0x30,0x04,0x01	},	"校时事件	"	}	,
		{	85	,{	0x03,0x30,0x05,0x00	},	"时段表编程次数"	}	,
		{	85	,{	0x03,0x30,0x05,0x01	},	"时段表编程"	}	,
		{	86	,{	0x03,0x30,0x06,0x00	},	"时区表编程次数"	}	,
		{	86	,{	0x03,0x30,0x06,0x01	},	"时区表编程"	}	,
		{	87	,{	0x03,0x30,0x07,0x00	},	"周休日编程次数"	}	,
		{	87	,{	0x03,0x30,0x07,0x01	},	"周休日编程"	}	,
		{	88	,{	0x03,0x30,0x08,0x00	},	"节假日编程次数"	}	,
		{	88	,{	0x03,0x30,0x08,0x01	},	"节假日编程"	}	,
		{	89	,{	0x03,0x30,0x09,0x00	},	"有功组合方式编程次数"	}	,
		{	89	,{	0x03,0x30,0x09,0x01	},	"有功组合方式编程"	}	,
		{	90	,{	0x03,0x30,0x0a,0x00	},	"无功组合方式 1 编程次数"	}	,
		{	90	,{	0x03,0x30,0x0a,0x01	},	"无功组合方式 1 编程"	}	,
		{	91	,{	0x03,0x30,0x0b,0x00	},	"无功组合方式 2 编程次数"	}	,
		{	91	,{	0x03,0x30,0x0b,0x01	},	"无功组合方式 2 编程"	}	,
		{	92	,{	0x03,0x30,0x0c,0x00	},	"结算日编程次数"	}	,
		{	92	,{	0x03,0x30,0x0c,0x01	},	"结算日编程"	}	,
		{	10	,{	0x03,0x30,0x0d,0x00	},	"开表盖事件次数"	}	,
		{	10	,{	0x03,0x30,0x0d,0x01	},	"开表盖事件"	}	,
		{	11	,{	0x03,0x30,0x0e,0x00	},	"开端纽盖事件次数"	}	,
		{	11	,{	0x03,0x30,0x0e,0x01	},	"开端纽盖事件"	}	,
		{	70	,{	0x03,0x12,0x00,0x00	},	"需量超限次数	"	}	,
		{	70	,{	0x03,0x12,0x01,0x01	},	"需量超限	"	}	,
		{	70	,{	0x03,0x12,0x02,0x01	},	"需量超限	"	}	,
		{	70	,{	0x03,0x12,0x03,0x01	},	"需量超限	"	}	,
		{	70	,{	0x03,0x12,0x04,0x01	},	"需量超限	"	}	,
		{	70	,{	0x03,0x12,0x05,0x01	},	"需量超限	"	}	,
		{	70	,{	0x03,0x12,0x06,0x01	},	"需量超限	"	}	,
		{	72	,{	0x20,0x00,0x00,0x01	},	"电流严重不平衡次数"	}	,
		{	72	,{	0x20,0x00,0x01,0x01	},	"电流严重不平衡"	}	,
		{	72	,{	0x20,0x00,0x13,0x01	},	"电流严重不平衡"	}	,
		{	73	,{	0x21,0x00,0x00,0x00	},	"潮流反向事件次数"		}	,
		{	73	,{	0x21,0x00,0x00,0x01	},	"潮流反向事件"		}	,
		{	93	,{	0x03,0x30,0x0f,0x00	},	"费率参数表编程次数"	}	,
		{	93	,{	0x03,0x30,0x0f,0x01	},	"费率参数表编程"	}	,
		{	94	,{	0x03,0x30,0x10,0x00	},	"阶梯表编程次数"	}	,
		{	94	,{	0x03,0x30,0x10,0x01	},	"阶梯表编程"	}	,
		{	95	,{	0x03,0x30,0x12,0x00	},	"密钥更新次数"	}	,
		{	95	,{	0x03,0x30,0x12,0x01	},	"密钥更新	"	}	,
		{	12	,{	0x03,0x35,0x00,0x00	},	"恒定磁场干扰次数	"	}	,
		{	12	,{	0x03,0x35,0x00,0x01	},	"恒定磁场干扰	"	}	,
		{	0	,{	0x03,0x36,0x00,0x00	},	"负荷开关误动作次数	"	}	,
		{	0	,{	0x03,0x36,0x00,0x01	},	"负荷开关误动作	"	}	,
		{	13	,{	0x03,0x37,0x00,0x00	},	"电源异常次数	"	}	,
		{	13	,{	0x03,0x37,0x00,0x01	},	"电源异常	"	}	,
		{	68	,{	0x03,0x06,0x00,0x00	},	"辅助电源失电次数	"	}	,
		{	68	,{	0x03,0x06,0x00,0x01	},	"辅助电源失电	"	}	,
};

//---------------------------------------------------------------

//typedef struct
//{
//	INT8U startIndex; 	//报文中的某数据的起始字节
//	INT8U dataLen;	 	//数据长度（字节数）
//	INT8U intbits;		//整数部分长度
//	INT8U decbits;		//小数部分长度
//	char name[30];
//	INT8U Flg07[4];//对应07表实时数据标识
//	OAD oad1;
//	OAD oad2;
//}MeterCurveDataType;//电表负荷记录
//typedef struct
//{
//	INT8U startIndex; 	//报文中的某数据的起始字节
//	INT8U dataLen;	 	//数据长度（字节数）
//}NDataType;//电表负荷记录

//int MCurveData[10];
//{
//						{8,  2, 3, 1, "A相电压",			{0x01,0x01,0x10,0x06}},
//						{10, 2, 3, 1, "B相电压",			{0x02,0x01,0x10,0x06}},
//						{12, 2, 3, 1, "C相电压",			{0x03,0x01,0x10,0x06}},
//
//						{14, 3, 3, 3, "A相电流",			{0x01,0x02,0x10,0x06}},
//						{17, 3, 3, 3, "B相电流",			{0x02,0x02,0x10,0x06}},
//						{20, 3, 3, 3, "C相电流",			{0x03,0x02,0x10,0x06}},
//
//						{23, 2, 2, 2, "频率曲线",			{0xFF,0xFF,0xFF,0xFF}},
//
//						{26, 3, 2, 4, "总有功功率曲线",	{0x00,0x03,0x10,0x06}},
//						{29, 3, 2, 4, "A相有功功率曲线",	{0x01,0x03,0x10,0x06}},
//						{32, 3, 2, 4, "B相有功功率曲线",	{0x02,0x03,0x10,0x06}},
//						{35, 3, 2, 4, "C相有功功率曲线",	{0x03,0x03,0x10,0x06}},
//
//						{38, 3, 2, 4, "总无功功率曲线",	{0x00,0x04,0x10,0x06}},
//						{41, 3, 2, 4, "A相无功功率曲线",	{0x01,0x04,0x10,0x06}},
//						{44, 3, 2, 4, "B相无功功率曲线",	{0x02,0x04,0x10,0x06}},
//						{47, 3, 2, 4, "C相无功功率曲线",	{0x03,0x04,0x10,0x06}},
//
//						{51, 2, 3, 1, "总功率因数曲线",	{0x00,0x05,0x10,0x06}},
//						{53, 2, 3, 1, "A相功率因数曲线",	{0x01,0x05,0x10,0x06}},
//						{55, 2, 3, 1, "B相功率因数曲线",	{0x02,0x05,0x10,0x06}},
//						{57, 2, 3, 1, "C相功率因数曲线",	{0x03,0x05,0x10,0x06}},
//
//						{60, 4, 6, 2, "正向有功总电能曲线",{0x01,0x06,0x10,0x06}},
//						{64, 4, 6, 2, "反向有功总电能曲线",{0x02,0x06,0x10,0x06}},
//						{68, 4, 6, 2, "正向无功总电能曲线",{0x03,0x06,0x10,0x06}},
//						{72, 4, 6, 2, "反向无功总电能曲线",{0x04,0x06,0x10,0x06}},
//
//						{77, 4, 6, 2, "一象限无功总电能曲线",{0x01,0x07,0x10,0x06}},
//						{81, 4, 6, 2, "二象限无功总电能曲线",{0x02,0x07,0x10,0x06}},
//						{85, 4, 6, 2, "三象限无功总电能曲线",{0x03,0x07,0x10,0x06}},
//						{89, 4, 6, 2, "四象限无功总电能曲线",{0x04,0x07,0x10,0x06}},
//
//						{94, 3, 2, 4, "当前有功需量曲线",	{0xFF,0xFF,0xFF,0xFF}},
//						{97, 3, 2, 4, "当前无功需量曲线",	{0xFF,0xFF,0xFF,0xFF}},
//};
typedef struct
{
	INT8U valid;
	INT8U count;
	INT8U counter;
}AutoReportWordsInfo;

typedef struct
{
	INT8U dataFlag[4];//事件标识
	INT8U len;
	INT8U data[256];//事件的数据
}AutoEvent;
AutoEvent autoEvent_Save[30];//暂存的事件
AutoReportWordsInfo autoReportWordInfo[96];//12字节主动上报状态字对应的每个事件是否发生及次数
INT8U autoReportWords[256];//电表主动上报状态字
INT8U autoEventCounter;//96个事件中，发生的事件个数
INT16U autoEventTimes;//事件发生的总次数


CLASS_6015 fangAn6015[20];
TASK_INFO taskinfo;
typedef struct
{
	TS nowts;
	TS oldts;
	int initflag;
	int redo;			//0:无动作   1:重启抄读  2:恢复抄读
	int comfd;
	int modeFlag;		// 1:集中器主导	0：路由主导（默认）
	int state_bak;					//运行状态备份
	int state;						//当前运行状态
	INT8U taskno;					//当前任务编号
	DateTimeBCD endtime;			//任务结束时间
	CJ_FANGAN fangAn;				//当前采集方案
	time_t send_start_time;			//发送开始时间
	CLASS_6035 result6035;			//采集任务监测
	INT8U masteraddr[6];			//主节点地址
	INT8U sendbuf[ZBBUFSIZE];		//发送缓存
	INT8U recvbuf[ZBBUFSIZE];		//接收缓存
	INT8U dealbuf[ZBBUFSIZE];		//待处理数据缓存
	FORMAT3762 format_Down;
	FORMAT3762 format_Up;
}RUNTIME_PLC;
AFN03_F10_UP module_info;
struct Tsa_Node
{
	TSA tsa;
	int tsa_index;
	INT8U protocol;
	INT8U usrtype;
	INT8U flag[8];
	INT8U curr_i;
	INT8U readnum;
	struct Tsa_Node *next;
};
struct Tsa_Node *tsa_head;
struct Tsa_Node *tsa_zb_head;
int tsa_zb_count;
int tsa_count;
/////////////////////////////////////////
int RecvHead;
int RecvTail;
int DataLen;
int SlavePointNum;
INT8U ZBMasterAddr[6];
int rec_step;
time_t oldtime1;
time_t newtime1;
INT8U buf645[BUFSIZE645];
#endif /* EVENTCALC_H_ */
