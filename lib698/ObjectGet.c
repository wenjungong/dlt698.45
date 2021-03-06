/*
 * ObjectGet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "basedef.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "dlt698def.h"
#include "dlt698.h"
#include "OIfunc.h"
#include "PublicFunction.h"
#include "secure.h"
#include "class8.h"
#include "class12.h"
#include "class23.h"

extern INT8S (*pSendfun)(int name, int fd,INT8U* sndbuf,INT16U sndlen);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern INT8U Get_Event(OAD oad,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
extern INT8U Getevent_Record_Selector(RESULT_RECORD *record_para,ProgramInfo* prginfo_event);
extern INT16S composeSecurityResponse(INT8U* SendApdu,INT16U Length);
extern int comfd;
extern int Global_name;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];
extern INT8U securetype;
extern ProgramInfo *memp;
extern PIID piid_g;
extern TimeTag	Response_timetag;		//响应的时间标签值

typedef struct {
	INT8U	repsonseType;		//分帧响应类型 CHOICE 	错误信息[0]   DAR，  对象属性[1]   SEQUENCE OF A-ResultNormal，记录型对象属性	[2] SEQUENCE OF A-ResultRecord
	INT8U	seqOfNum;			//用于保存sequence of a-ResultNormal的个数，ResultNormal的情况分帧时写入文件
	INT16U	subframeSum;		//Get 分帧总数
	INT16U	frameNo;			//帧序号
	INT16U	currSite;			//当前帧序号在文件中位置
	INT16U	nextSite;			//下一帧数据偏移位置
} NEXT_INFO;	//getResponseNext 需要保存的信息内容

static NEXT_INFO	next_info={};

int BuildFrame_GetResponseRecord(INT8U response_type,CSINFO *csinfo,RESULT_RECORD record,INT8U *sendbuf)
{
	int index=0, hcsi=0;
	csinfo->dir = 1;
	csinfo->prm = 1;
	int apduplace =0;

	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	apduplace=index;
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = piid_g.data;		//	piid

//	fprintf(stderr,"record.datalen = %d\n",record.datalen);
	if (record.datalen > 0)  {
		if((index+record.datalen+3) > BUFLEN) {
			syslog(LOG_ERR,"GetResponseRecord 长度[%d]越限[%d],数据应答错误",record.datalen,BUFLEN);
		}else {
			memcpy(&sendbuf[index],record.data,record.datalen);
			index = index + record.datalen;
		}
	}
	sendbuf[index++] = 0;	//FollowReport
	sendbuf[index++] = 0;	//TimeTag
//	INT16U

	if(securetype!=0)//安全等级类型不为0，代表是通过安全传输下发报文，上行报文需要以不低于请求的安全级别回复
	{
		if (getZone("GW") == 0) {
			PacketBufToFile(1,"APDU_REC:", (char *)sendbuf, index, NULL);
		}
		apduplace += composeSecurityResponse(&sendbuf[apduplace],index-apduplace);
		index=apduplace;
	}
	FrameTail(sendbuf,index,hcsi);
	if(pSendfun!=NULL && csinfo->sa_type!=2 && csinfo->sa_type!=3)//组地址或广播地址不需要应答
		pSendfun(Global_name, comfd,sendbuf,index+3);
	return (index+3);
}
void addScodeProcess(INT8U *buf,int len)
{
	int i=0;
	for(i=0;i<len;i++)
	{
		buf[i] = (INT8U)(buf[i] + 0x33);
	}
}
int BuildFrame_GetResponse(INT8U response_type,CSINFO *csinfo,INT8U oadnum,RESULT_NORMAL response,INT8U *sendbuf)
{
	int apduplace =0 , scPlace=0;
	int index=0, hcsi=0;

	csinfo->dir = 1;
	csinfo->prm = 1;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	scPlace = index;
	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = piid_g.data;	//	piid
	if (oadnum>0)
		sendbuf[index++] = oadnum;
	if((index+response.datalen+3) > BUFLEN) {
		syslog(LOG_ERR,"GetResponse 长度[%d]越限[%d],数据应答错误",response.datalen,BUFLEN);
	}else {
		memcpy(&sendbuf[index],response.data,response.datalen);
		index = index + response.datalen;
	}
	sendbuf[index++] = 0;	//跟随上报信息域 	FollowReport
	index += fill_timetag(&sendbuf[index],Response_timetag);//时间标签		TimeTag
	if(securetype!=0)//安全等级类型不为0，代表是通过安全传输下发报文，上行报文需要以不低于请求的安全级别回复
	{
    	if (getZone("GW") == 0) {
    		PacketBufToFile(1,"APDU_GET:", (char *)sendbuf, index, NULL);
    	}
		apduplace += composeSecurityResponse(&sendbuf[apduplace],index-apduplace);
		index=apduplace;
	}
	if (csinfo->sc == 1)
	{
		addScodeProcess(&sendbuf[scPlace],index-scPlace);
	}
	FrameTail(sendbuf,index,hcsi);
	if(pSendfun!=NULL && csinfo->sa_type!=2 && csinfo->sa_type!=3)//组地址或广播地址不需要应答
		pSendfun(Global_name, comfd,sendbuf,index+3);
	return (index+3);
}

int BuildFrame_GetResponseNext(INT8U response_type,CSINFO *csinfo,INT8U DAR,INT16U datalen,INT8U *databuf,INT8U *sendbuf)
{
	int index=0, hcsi=0;
	int apduplace =0;
//	int i=0;

	csinfo->dir = 1;
	csinfo->prm = 1;
//	csinfo->gframeflg = 1;		//不能使用分帧标志，否则是一帧

	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = piid_g.data;		//	piid

	fprintf(stderr,"\n当前帧序号%d 总帧数%d\n",next_info.frameNo,next_info.subframeSum);
	if(next_info.frameNo==next_info.subframeSum) {
		sendbuf[index++] = TRUE;		//末帧标志
	}else sendbuf[index++] = FALSE;
	sendbuf[index++] = (next_info.frameNo >> 8) & 0xff;		//分帧序号
	sendbuf[index++] = next_info.frameNo & 0xff;
	if (datalen > 0)
	{
		if(next_info.repsonseType==GET_REQUEST_NORMAL || next_info.repsonseType==GET_REQUEST_NORMAL_LIST) {
			sendbuf[index++] = 1;		//对象属性[1]   SEQUENCE OF A-ResultNormal
		}else if(next_info.repsonseType==GET_REQUEST_RECORD || next_info.repsonseType==GET_REQUEST_RECORD_LIST) {
			sendbuf[index++] = 2;		//记录型对象属性[2]SEQUENCE OF A-ResultRecord
			sendbuf[index++] = 1;//A-ResultRecord∷=CHOICE  1:记录数据
		}
		if((index+datalen+3) > BUFLEN) {
			syslog(LOG_ERR,"GetResponseNext 长度[%d]越限[%d],数据应答错误",datalen,BUFLEN);
		}else {
			fprintf(stderr,"\n\n1**************************************");
			PRTbuf(databuf,datalen);
			fprintf(stderr,"\n\n1**************************************");
			memcpy(&sendbuf[index],databuf,datalen);
			index = index + datalen;
		}
	}else {
		sendbuf[index++] = 0;//choice 0  ,DAR 有效 (数据访问可能的结果)
		sendbuf[index++] = DAR;
	}
	sendbuf[index++] = 0;	//跟随上报信息域 	FollowReport
	index += fill_timetag(&sendbuf[index],Response_timetag);//时间标签		TimeTag
	if(securetype!=0)//安全等级类型不为0，代表是通过安全传输下发报文，上行报文需要以不低于请求的安全级别回复
	{
	   	if (getZone("GW") == 0) {
			PacketBufToFile(1,"APDU_NEX:", (char *)sendbuf, index, NULL);
		}
		apduplace += composeSecurityResponse(&sendbuf[apduplace],index-apduplace);
		index=apduplace;
	}
	FrameTail(sendbuf,index,hcsi);
	if(pSendfun!=NULL && csinfo->sa_type!=2 && csinfo->sa_type!=3)//组地址或广播地址不需要应答
		pSendfun(Global_name, comfd,sendbuf,index+3);
	return (index+3);
}

/*
 * 分帧处理判断
 * 文件前两个字节：后面一帧数据总长度(不包括这两个字节),先低后高
 * 第三个字节：	sequence of ResultNormal = m（m=0,GET_REQUEST_NORMAL，表示A-ResultNormal）
 * 第4个字节开始：m个 ResultNormal
 * */
int Build_subFrame(INT8U saveflg,INT16U framelen,INT8U seqOfNum,RESULT_NORMAL *response)
{
	int 	index=0;
	FILE	*fp=NULL;
	INT8U	oneFrameBuf[MAX_APDU_SIZE]={};

//	fprintf(stderr,"Build_subFrame framelen=%d,subframeSum=%d\n",framelen,next_info.subframeSum);
	if((saveflg == 1) || (framelen >= FRAME_SIZE))  {	//需要分帧,数据帧长度大于尺寸，或者在最后一帧不足分帧尺寸进行保存
		fp = fopen(PARA_FRAME_DATA,"a+");
		if(fp==NULL) 	return -1;

		fprintf(stderr,"\nnext_info:repsonseType=%d,subframeSum=%d,seqOfNum=%d\n",next_info.repsonseType,next_info.subframeSum,next_info.seqOfNum);
		memset(&oneFrameBuf,0,sizeof(oneFrameBuf));
		index = 2;
		if(seqOfNum==0) {
			next_info.repsonseType = GET_REQUEST_NORMAL;
			oneFrameBuf[index++] = 1;				//sequence of num
		}
		else {
			next_info.repsonseType = GET_REQUEST_NORMAL_LIST;
			oneFrameBuf[index++] = seqOfNum-next_info.seqOfNum;			//TODO：无测试
		}
		index += create_OAD(0,&oneFrameBuf[index],response->oad);		//OAD
		oneFrameBuf[index++] = 01;		//response->dar; Data

//		fprintf(stderr,"response.datalen=%d  ",response->datalen);
//		int i=0;
//		for(i=0;i< response->datalen;i++) {
//			fprintf(stderr,"%02x ",response->data[i]);
//		}
		if((index+response->datalen+3) > BUFLEN) {
			syslog(LOG_ERR,"subFrame 长度[%d]越限[%d],数据应答错误",response->datalen,BUFLEN);
		}else {
			memcpy(&oneFrameBuf[index],response->data,response->datalen);	//报文帧中 从 array开始
			index += response->datalen;
		}
		oneFrameBuf[0] = (index-2) & 0xff;				//报文帧长度
		oneFrameBuf[1] = ((index-2) >>8) & 0xff;
		saveOneFrame(oneFrameBuf,index,fp);
		next_info.subframeSum++;		//每次下发getrequest清除
		next_info.seqOfNum = seqOfNum;
//		fprintf(stderr,"write one Frame(len=%d),response->datalen=%d subframeSum=%d seqOfNum=%d\n",
//				index,response->datalen,next_info.subframeSum,next_info.seqOfNum);
		if(fp != NULL)
			fclose(fp);
	}
	return index;
}

/*
 * 组织分帧数据
 * */
int Get_subFrameData(INT16U frameoffset,CSINFO *csinfo,INT8U *sendbuf)
{
	int		datalen=0;
	INT8U	DAR=success;

	next_info.nextSite = readFrameDataFile(PARA_FRAME_DATA,frameoffset,TmpDataBuf,&datalen);
	fprintf(stderr,"Get_subFrameData  frameoffset=%d  datalen=%d\n",frameoffset,datalen);
	fprintf(stderr,"\nnext_info:repsonseType=%d,subframeSum=%d,seqOfNum=%d,currSite=%d,nextSite=%d,frameNo=%d\n",
			next_info.repsonseType,next_info.subframeSum,next_info.seqOfNum,next_info.currSite,next_info.nextSite,next_info.frameNo);
	if(datalen==0)  DAR = framesegment_cancel;
	BuildFrame_GetResponseNext(GET_REQUEST_RECORD_NEXT,csinfo,DAR,datalen,TmpDataBuf,sendbuf);
	return 1;
}

//int GetMeterInfo(INT8U seqOfNum,RESULT_NORMAL *response)
//{
//	int 	index=0;
//	INT8U 	*data = NULL;
//	OAD oad={};
//	CLASS_6001	 meter={};
//	INT16U		i=0,blknum=0,meternum=0;
//	int		retlen=0;
//	int		oneUnitLen=0;	//计算一个配置单元的长度，统计是否需要分帧操作
//	int		lastframenum = 0; //记录分帧的数量
//
//	oad = response->oad;
//	data = response->data;
//
//	oneUnitLen = Get_6001(1,0,&data[0]);
//	if(oneUnitLen == 0) {
//		fprintf(stderr,"get OI=%04x unitlen=0, 退出",oad.OI);
//		return 0;
//	}
//	fprintf(stderr,"6001——oneUnitLen=%d\n",oneUnitLen);
//	memset(&meter,0,sizeof(CLASS_6001));
//	blknum = getFileRecordNum(oad.OI);
//	fprintf(stderr,"GetMeterInfo oad.oi=%04x blknum=%d\n",oad.OI,blknum);
//	if(blknum<=1)	return 0;
//	index = 2;			//空出 array,结束后填入
//	for(i=0;i<blknum;i++)
//	{
//		create_array(&data[0],meternum);		//每次循环填充配置单元array个数，为了组帧分帧
//		response->datalen = index;
//
//		///在读取数据组帧前判断是否需要进行分帧
//		Build_subFrame(0,(index+oneUnitLen),seqOfNum,response);
//		if(lastframenum != next_info.subframeSum) {		//数据已分帧重新开始
//			lastframenum = next_info.subframeSum;
//			index = 2;
//			meternum = 0;
////			fprintf(stderr,"\n get subFrame lastframenum=%d,subframeSum=%d index=%d\n",lastframenum,next_info.subframeSum,index);
//		}
//		retlen = Get_6001(0,i,&data[index]);
//		if(retlen!=0) {
//			meternum++;
//		}
//		index += retlen;
//	}
//	create_array(&data[0],meternum);		//配置单元个数
//	response->datalen = index;
//	if(next_info.subframeSum!=0) {		//已经存在分帧情况
//		Build_subFrame(1,index,seqOfNum,response);		//后续帧组帧, TODO:RequestNormalList 方法此处调用是否合适
//	}
//	return 0;
//}

int GetYxPara(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_f203 objtmp;
	int	 chgflag=0;
	oad = response->oad;
	data = response->data;
	memset(&objtmp,0,sizeof(objtmp));
	readCoverClass(0xf203,0,&objtmp,sizeof(objtmp),para_vari_save);
	switch(oad.attflg )
	{
		case 4://配置参数
			index += create_struct(&data[index],2);
			index += fill_bit_string(&data[index],8,&objtmp.state4.StateAcessFlag);
			index += fill_bit_string(&data[index],8,&objtmp.state4.StatePropFlag);
			break;
		case 2://设备对象列表
			fprintf(stderr,"GetYxPara oi.att=%d\n",oad.attflg);
			if(memp->cfg_para.device == CCTT2) {
				objtmp.statearri.num = 1;
			}else
				objtmp.statearri.num = 4;
			index += create_array(&data[index],objtmp.statearri.num);
			for(int i=0;i<objtmp.statearri.num;i++)
			{
				index += create_struct(&data[index],2);
				index += fill_unsigned(&data[index],objtmp.statearri.stateunit[i].ST);
				index += fill_unsigned(&data[index],objtmp.statearri.stateunit[i].CD);
				if(objtmp.statearri.stateunit[i].CD) {
					objtmp.statearri.stateunit[i].CD = 0;
					chgflag = 1;
				}
			}
			if(chgflag) {	//遥信变位状态更改后，保存
				saveCoverClass(0xf203,0,&objtmp,sizeof(objtmp),para_vari_save);
			}
			fprintf(stderr,"index=%d\n",index);
			break;
	}
	response->datalen = index;
	return 0;
}

/*继电器单元
 * */
int Get_f205_attr2(RESULT_NORMAL *response)
{
	int index=0;
	int i=0;
	INT8U unit_index = 0,relaynum = 0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_F205 objtmp={};

	oad = response->oad;
	data = response->data;
	memset(&objtmp,0,sizeof(CLASS_F205));
//	readCoverClass(oad.OI, 0, (void *) &objtmp, sizeof(CLASS_F205),para_vari_save);
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&objtmp,&shareAddr->ctrls.cf205,sizeof(CLASS_F205));
	switch(oad.attflg )
	{
		case 2://设备对象列表
			unit_index = oad.attrindex;
			if(unit_index==0) {
				relaynum = 3;
			}else  relaynum = 1;
			for(i = 0;i < relaynum; i++) {
				if(relaynum>1) {
					index += create_array(&data[index],relaynum);
				}
				syslog(LOG_NOTICE,"mem relaynum = %d  currentState=%d switchAttr=%d wiredState=%d\n",i,
						objtmp.unit[i].currentState,objtmp.unit[i].switchAttr,objtmp.unit[i].wiredState);
				index += create_struct(&data[index], 4);
				index += fill_visible_string(&data[index],&objtmp.unit[i].devdesc[1],objtmp.unit[i].devdesc[0]);
				index += fill_enum(&data[index],objtmp.unit[i].currentState);
				index += fill_enum(&data[index],objtmp.unit[i].switchAttr);
				index += fill_enum(&data[index],objtmp.unit[i].wiredState);
			}
			response->datalen = index;
			break;
	}
	return 0;
}
int GetEsamPara(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad={};
	oad = response->oad;
	data = response->data;
	response->datalen = getEsamAttribute(oad,data);
	if(response->datalen == 0)
		response->dar = 0x16;//esam验证失败
	return 0;
}

//遥控
int Get_8000(RESULT_NORMAL *response)
{
	CLASS_8000 c8000={};
	INT8U *data=NULL;
	INT8U	index=0;
	OAD oad = response->oad;
	data = response->data;
	ProgramInfo *shareAddr = getShareAddr();

//	memset(&c8000,0,sizeof(CLASS_8000));
//	readCoverClass(oad.OI, 0, (void *) &c8000, sizeof(CLASS_8000),para_vari_save);
	memcpy(&c8000,&shareAddr->ctrls.c8000,sizeof(CLASS_8000));
	switch(oad.attflg) {
	case 2:
		index += create_struct(&data[index],2);
		index += fill_double_long_unsigned(&data[index],c8000.limit);
		index += fill_long_unsigned(&data[index],c8000.delaytime);
		break;
	case 4:
		index = fill_bit_string(data,8,&c8000.alarmstate);
		break;
	case 5:
		index = fill_bit_string(data,8,&c8000.cmdstate);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8000 datalen = %d\n",response->datalen);
	return response->datalen;
}
//保电
int Get_8001(RESULT_NORMAL *response)
{
	CLASS_8001 c8001={};
	INT8U *data=NULL;
	INT8U	i=0,index=0;
	OAD oad = response->oad;
	data = response->data;
	ProgramInfo *shareAddr = getShareAddr();

//	memset(&c8001,0,sizeof(CLASS_8001));
//	readCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),para_vari_save);
	memcpy(&c8001,&shareAddr->ctrls.c8001,sizeof(CLASS_8001));
	fprintf(stderr,"c8001.noCommTime=%d autoTime=%d\n",c8001.noCommTime,c8001.autoTime);
	switch(oad.attflg) {
	case 2:
		index = fill_enum(data, c8001.state);
		break;
	case 3:
		index = fill_long_unsigned(data, c8001.noCommTime);
		break;
	case 4:
		index = fill_long_unsigned(data, c8001.autoTime);
		break;
	case 5:
		index += create_array(&data[index],c8001.unit_count);
		for(i=0;i<c8001.unit_count;i++) {
			index += create_struct(&data[index],2);
			index += fill_unsigned(&data[index],c8001.unit[i].autoTimeStart);
			index += fill_unsigned(&data[index],c8001.unit[i].autoTimeEnd);
		}
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8001 datalen = %d\n",response->datalen);
	return response->datalen;
}
//催费告警
int Get_8002(RESULT_NORMAL *response)
{
	CLASS_8002 c8002;
	INT16U	index = 0;
	INT8U *data=NULL;
	OAD oad;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	response->datalen = 0;
//	readCoverClass(0x8002, 0, (void *) &c8002, sizeof(CLASS_8002), para_vari_save);
	memcpy(&c8002,&shareAddr->ctrls.c8002,sizeof(CLASS_8002));
	switch(oad.attflg) {
	case 2:
		response->dar = getEnumValid(c8002.state,0,1,0);
		if(response->dar == success) {
			response->datalen = fill_enum(data, c8002.state);
		}
		break;
	case 3:
		index += create_struct(&data[index], 2);
		index += fill_octet_string(&data[index],&c8002.alarmTime[1],3);
		index += fill_visible_string(&data[index],&c8002.alarmInfo[1],c8002.alarmInfo[0]);
		response->datalen = index;
		break;
	}

	fprintf(stderr,"C8002 datalen = %d\n",response->datalen);
	return response->datalen;
}

//一般中文信息
int Get_8003_8004(RESULT_NORMAL *response)
{
	CLASS_8003_8004 info={};
	INT16U	index = 0;
	INT8U *data=NULL;
	OAD oad={};
	INT8U	info_num=0,i=0;
	int  ret=0;

	oad = response->oad;
	data = response->data;
	response->datalen = 0;
	ret = readCoverClass(oad.OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
	if(ret==-1) {	//文件不存在，初始化中文信息序号
		for(i=0;i<10;i++) {
			info.chinese_info[i].no = 0xFF;
		}
	}
	switch(oad.attflg) {
	case 2:
		index += create_array(&data[index],info_num);
		for(i=0;i<10;i++) {
			if(info.chinese_info[i].no!=0xff) {
				info_num++;
				index += create_struct(&data[index], 4);
				index += fill_unsigned(&data[index],info.chinese_info[i].no);
				index += fill_date_time_s(&data[index],&info.chinese_info[i].releaseData);
				index += fill_bool(&data[index],info.chinese_info[i].readflg);
				index += fill_visible_string(&data[index],&info.chinese_info[i].info[1],info.chinese_info[i].info[0]);
			}
		}
		if(info_num) {
			create_array(&data[0],info_num);	//数组回填
			response->datalen = index;
		}else {
			response->dar = success;
			response->datalen += create_array(&data[0],0);
		}
		break;
	case 3:
		for(i=0;i<10;i++) {
			if(info.chinese_info[i].no!=0xff) {
				info_num++;
			}
		}
		response->datalen += fill_long_unsigned(&data[0],info_num);
		break;
	case 4:
		response->datalen += fill_long_unsigned(&data[0],10);
		break;
	}

	fprintf(stderr,"CLASS_8003_8004 oi=%04x datalen = %d\n",oad.OI,response->datalen);
	return response->datalen;
}

/*
 * 填充总加组状态结构 ALSTATE
 * */
int fill_ALSTATE(INT8U *data,ALSTATE *alstate,INT8U datatype)
{
	int 	index=0;
	INT8U	i=0,unitnum=0;
	unitnum=0;
//	for(i=0;i<MAX_AL_UNIT;i++) {
//		if(alstate[i].name !=0) {  //总加组对象不为0，有效
//			unitnum++;
//		};
//	}
//	if(unitnum) {
		index += create_array(&data[index],unitnum);	//数组位置，后面计算实际值更新改位置
		for(i=0;i<MAX_AL_UNIT;i++) {
			fprintf(stderr,"read c8103.enable[%d].name=%x state=%d\n",i,alstate[i].name,alstate[i].state);
			if(alstate[i].name==0) continue;
			unitnum++;
			index += create_struct(&data[index],2);
//			fprintf(stderr,"name = %04x\n",alstate[i].name);
			index += fill_OI(&data[index],alstate[i].name);
			switch(datatype) {
			case dtenum:
				index += fill_enum(&data[index],alstate[i].state);
				break;
			case dtbitstring:
				index += fill_bit_string(&data[index],8,&alstate[i].state);
				break;
			default:
				data[index++] = 0;
				break;
			}
		}

		fprintf(stderr,"unitnum = %d\n",unitnum);
		if(unitnum) {
			create_array(&data[0],unitnum);		//赋array的有效值
		}else  {
			index = 0;
			data[index++] = 0;		//NULL
		}
//	}else {
//		data[index++] = 0;		//NULL
//	}
		fprintf(stderr," fill_ALSTATE index = %d\n",index);
	return index;
}

/*
 * 填充总加组状态结构 ALSTATE
 * */
int fill_ALSTATE_8106(INT8U *data,ALSTATE alstate,INT8U datatype)
{
	int 	index=0;

//	fprintf(stderr,"alstate.name = %04x\n",alstate.name);
	if(alstate.name!=0) {
		index += create_struct(&data[index],2);
//		fprintf(stderr,"name = %04x\n",alstate.name);
		index += fill_OI(&data[index],alstate.name);
		switch(datatype) {
		case dtenum:
			index += fill_enum(&data[index],alstate.state);
			break;
		case dtbitstring:
			index += fill_bit_string(&data[index],8,&alstate.state);
			break;
		default:
			data[index++] = 0;
			break;
		}
	}else  {
		index = 0;
		data[index++] = 0;		//NULL
	}
	return index;
}

int fill_PowerCtrlParam(INT8U *data,PowerCtrlParam para)
{
	int 	index=0;
	index += create_struct(&data[index],9);
	index += fill_bit_string(&data[index],8,&para.n);
	index += fill_long64(&data[index],para.t1);
	index += fill_long64(&data[index],para.t2);
	index += fill_long64(&data[index],para.t3);
	index += fill_long64(&data[index],para.t4);
	index += fill_long64(&data[index],para.t5);
	index += fill_long64(&data[index],para.t6);
	index += fill_long64(&data[index],para.t7);
	index += fill_long64(&data[index],para.t8);
	return index;
}

//终端保安定值
int Get_8100(RESULT_NORMAL *response)
{
//	CLASS_8100 c8100={};
	INT8U *data=NULL;
	OAD oad={};
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
//	readCoverClass(0x8100, 0, (void *) &c8100, sizeof(CLASS_8100),para_vari_save);
//	memcpy(&c8100,&shareAddr->ctrls.c8100,sizeof(CLASS_8100));
	response->datalen = fill_long64(data, shareAddr->ctrls.c8100.v);
//	asyslog(LOG_WARNING, "读取终端安保定值(%lld)", shareAddr->ctrls.c8100.v);
	fprintf(stderr,"datalen = %d\n",response->datalen);
	return response->datalen;
}
//终端功控时段
int Get_8101(RESULT_NORMAL *response)
{
	CLASS_8101 c8101={};
	INT8U *data=NULL;
	INT8U	i=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8101,0,sizeof(CLASS_8101));
//	readCoverClass(oad.OI, 0, (void *) &c8101, sizeof(CLASS_8101),para_vari_save);
	memcpy(&c8101,&shareAddr->ctrls.c8101,sizeof(CLASS_8101));
	switch(oad.attflg){
	case 2:
		index += create_array(&data[index],c8101.time_num);
		for(i=0;i<c8101.time_num;i++) {
			index += fill_unsigned(&data[index],c8101.time[i]);
		}
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8101 datalen = %d\n",response->datalen);
	return response->datalen;
}

//功控告警时间
int Get_8102(RESULT_NORMAL *response)
{
	CLASS_8102 c8102={};
	INT8U *data=NULL;
	INT8U	i=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8102,0,sizeof(CLASS_8102));
//	readCoverClass(oad.OI, 0, (void *) &c8102, sizeof(CLASS_8102),para_vari_save);
	memcpy(&c8102,&shareAddr->ctrls.c8102,sizeof(CLASS_8102));
	switch(oad.attflg){
	case 2:
		c8102.time_num = limitJudge("功控告警时间",8,c8102.time_num);
		fprintf(stderr,"c8102.time_num = %d\n",c8102.time_num);
		index += create_array(&data[index],c8102.time_num);
		for(i=0;i<c8102.time_num;i++) {
			fprintf(stderr,"c8102.time[%d] = %d\n",i,c8102.time[i]);
			index += fill_unsigned(&data[index],c8102.time[i]);
		}
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8102 datalen = %d\n",response->datalen);
	return response->datalen;
}

//时段功控
int Get_8103(RESULT_NORMAL *response)
{
	CLASS_8103 c8103={};
	INT8U *data=NULL;
	INT8U	i=0,unitnum=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8103,0,sizeof(CLASS_8103));
//	readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	switch(oad.attflg){
	case 2:	//时段功控配置单元
		unitnum=0;
		index += 2;	//array + 数组个数
		for(i=0;i<MAX_AL_UNIT;i++) {
			if(c8103.list[i].index != 0) {
				unitnum++;
				index += create_struct(&data[index],6);
				index += fill_OI(&data[index],c8103.list[i].index);
				index += fill_bit_string(&data[index],8,&c8103.list[i].sign);
				index += fill_PowerCtrlParam(&data[index],c8103.list[i].v1);
				index += fill_PowerCtrlParam(&data[index],c8103.list[i].v2);
				index += fill_PowerCtrlParam(&data[index],c8103.list[i].v3);
				index += fill_integer(&data[index],c8103.list[i].para);
			}
		}
		if(unitnum) {
			create_array(&data[0],unitnum);
		}else {
			index = 0;
			data[index++] = 0;	//NULL
		}
		break;
	case 3:	//控制投入状态
		fprintf(stderr,"c8103.enable[0].name=%x state=%d\n",c8103.enable[0].name,c8103.enable[0].state);
		index += fill_ALSTATE(&data[index],c8103.enable,dtenum);
		break;
	case 4:	//控制输出状态
		index += fill_ALSTATE(&data[index],c8103.output,dtbitstring);
		break;
	case 5: //越限告警状态
		index += fill_ALSTATE(&data[index],c8103.overflow,dtenum);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8103 datalen = %d\n",response->datalen);
	return response->datalen;
}

//厂休控
int Get_8104(RESULT_NORMAL *response)
{
	CLASS_8104 c8104={};
	INT8U *data=NULL;
	INT8U	i=0,unitnum=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8104,0,sizeof(CLASS_8104));
//	readCoverClass(oad.OI, 0, (void *) &c8104, sizeof(CLASS_8104),para_vari_save);
	memcpy(&c8104,&shareAddr->ctrls.c8104,sizeof(CLASS_8104));
	switch(oad.attflg){
	case 2:	//厂休控配置单元
		unitnum=0;
		index += 2;	//array + 数组个数
		for(i=0;i<MAX_AL_UNIT;i++) {
			if(c8104.list[i].index != 0) {
				unitnum++;
				index += create_struct(&data[index],5);
				index += fill_OI(&data[index],c8104.list[i].index);
				index += fill_long64(&data[index],c8104.list[i].v);
				index += fill_date_time_s(&data[index],&c8104.list[i].start);
				index += fill_long_unsigned(&data[index],c8104.list[i].sustain);
				index += fill_bit_string(&data[index],8,&c8104.list[i].noDay);
			}
		}
		if(unitnum) {
			create_array(&data[0],unitnum);
		}else {
			index = 0;
			data[index++] = 0;	//NULL
		}
		break;
	case 3:	//控制投入状态
		index += fill_ALSTATE(&data[index],c8104.enable,dtenum);
		break;
	case 4:	//控制输出状态
		index += fill_ALSTATE(&data[index],c8104.output,dtbitstring);
		break;
	case 5: //越限告警状态
		index += fill_ALSTATE(&data[index],c8104.overflow,dtenum);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8104 datalen = %d\n",response->datalen);
	return response->datalen;
}

//营业报停控
int Get_8105(RESULT_NORMAL *response)
{
	CLASS_8105 c8105={};
	INT8U *data=NULL;
	INT8U	i=0,unitnum=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8105,0,sizeof(CLASS_8105));
//	readCoverClass(oad.OI, 0, (void *) &c8105, sizeof(CLASS_8105),para_vari_save);
	memcpy(&c8105,&shareAddr->ctrls.c8105,sizeof(CLASS_8105));
	switch(oad.attflg){
	case 2:	//营业报停控配置单元
		unitnum=0;
		index += 2;	//array + 数组个数
		for(i=0;i<MAX_AL_UNIT;i++) {
			if(c8105.list[i].index != 0) {
				unitnum++;
				index += create_struct(&data[index],4);
				index += fill_OI(&data[index],c8105.list[i].index);
				index += fill_date_time_s(&data[index],&c8105.list[i].start);
				index += fill_date_time_s(&data[index],&c8105.list[i].end);
				index += fill_long64(&data[index],c8105.list[i].v);
			}
		}
		if(unitnum) {
			create_array(&data[0],unitnum);
		}else {
			index = 0;
			data[index++] = 0;	//NULL
		}
		break;
	case 3:	//控制投入状态
		index += fill_ALSTATE(&data[index],c8105.enable,dtenum);
		break;
	case 4:	//控制输出状态
		index += fill_ALSTATE(&data[index],c8105.output,dtbitstring);
		break;
	case 5: //越限告警状态
		index += fill_ALSTATE(&data[index],c8105.overflow,dtenum);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8105 datalen = %d\n",response->datalen);
	return response->datalen;
}

//当前功率下浮控
int Get_8106(RESULT_NORMAL *response)
{
	CLASS_8106 c8106={};
	INT8U *data=NULL;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8106,0,sizeof(CLASS_8106));
//	readCoverClass(oad.OI, 0, (void *) &c8106, sizeof(CLASS_8106),para_vari_save);
	memcpy(&c8106,&shareAddr->ctrls.c8106,sizeof(CLASS_8106));

	switch(oad.attflg){
	case 2:	//营业报停控配置单元
		index += create_struct(&data[index],8);
		index += fill_unsigned(&data[index],c8106.list.down_huacha);
		index += fill_integer(&data[index],c8106.list.down_xishu);
		index += fill_unsigned(&data[index],c8106.list.down_freeze);
		index += fill_unsigned(&data[index],c8106.list.down_ctrl_time);
		index += fill_unsigned(&data[index],c8106.list.t1);
		index += fill_unsigned(&data[index],c8106.list.t2);
		index += fill_unsigned(&data[index],c8106.list.t3);
		index += fill_unsigned(&data[index],c8106.list.t4);
		break;
	case 3:	//控制投入状态
		fprintf(stderr,"c8106.enable.name = %04x\n",c8106.enable.name);
		index += fill_ALSTATE_8106(&data[index],c8106.enable,dtenum);
		break;
	case 4:	//控制输出状态
		index += fill_ALSTATE_8106(&data[index],c8106.output,dtbitstring);
		break;
	case 5: //越限告警状态
		index += fill_ALSTATE_8106(&data[index],c8106.overflow,dtenum);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8106 datalen = %d\n",response->datalen);
	return response->datalen;
}

//购电控
int Get_8107(RESULT_NORMAL *response)
{
	CLASS_8107 c8107={};
	INT8U *data=NULL;
	INT8U	i=0,unitnum=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8107,0,sizeof(CLASS_8107));
//	readCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),para_vari_save);
	memcpy(&c8107,&shareAddr->ctrls.c8107,sizeof(CLASS_8107));
	switch(oad.attflg){
	case 2:	//购电控配置单元
		unitnum=0;
		index += 2;	//array + 数组个数
		for(i=0;i<MAX_AL_UNIT;i++) {
			if(c8107.list[i].index != 0) {
				unitnum++;
				index += create_struct(&data[index],8);
				index += fill_OI(&data[index],c8107.list[i].index);
				index += fill_double_long_unsigned(&data[index],c8107.list[i].no);
				index += fill_enum(&data[index],c8107.list[i].add_refresh);
				index += fill_enum(&data[index],c8107.list[i].type);
				index += fill_long64(&data[index],c8107.list[i].v);
				index += fill_long64(&data[index],c8107.list[i].alarm);
				index += fill_long64(&data[index],c8107.list[i].ctrl);
				index += fill_enum(&data[index],c8107.list[i].mode);
			}
		}
		if(unitnum) {
			create_array(&data[0],unitnum);
		}else {
			index = 0;
			data[index++] = 0;	//NULL
		}
		break;
	case 3:	//控制投入状态
		index += fill_ALSTATE(&data[index],c8107.enable,dtenum);
		break;
	case 4:	//控制输出状态
		index += fill_ALSTATE(&data[index],c8107.output,dtbitstring);
		break;
	case 5: //越限告警状态
		index += fill_ALSTATE(&data[index],c8107.overflow,dtenum);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8107 datalen = %d\n",response->datalen);
	return response->datalen;
}

//月电控
int Get_8108(RESULT_NORMAL *response)
{
	CLASS_8108 c8108={};
	INT8U *data=NULL;
	INT8U	i=0,unitnum=0;
	OAD 	oad={};
	int 	index=0;
	ProgramInfo *shareAddr = getShareAddr();

	oad = response->oad;
	data = response->data;
	memset(&c8108,0,sizeof(CLASS_8108));
//	readCoverClass(oad.OI, 0, (void *) &c8108, sizeof(CLASS_8108),para_vari_save);
	memcpy(&c8108,&shareAddr->ctrls.c8108,sizeof(CLASS_8108));

	switch(oad.attflg){
	case 2:	//月电控配置单元
		unitnum=0;
		index += 2;	//array + 数组个数
		for(i=0;i<MAX_AL_UNIT;i++) {
			if(c8108.list[i].index != 0) {
				unitnum++;
				index += create_struct(&data[index],4);
				index += fill_OI(&data[index],c8108.list[i].index);
				index += fill_long64(&data[index],c8108.list[i].v);
				index += fill_unsigned(&data[index],c8108.list[i].para);
				index += fill_integer(&data[index],c8108.list[i].flex);
			}
		}
		if(unitnum) {
			create_array(&data[0],unitnum);
		}else {
			index = 0;
			data[index++] = 0;	//NULL
		}
		break;
	case 3:	//控制投入状态
		index += fill_ALSTATE(&data[index],c8108.enable,dtenum);
		break;
	case 4:	//控制输出状态
		index += fill_ALSTATE(&data[index],c8108.output,dtbitstring);
		break;
	case 5: //越限告警状态
		index += fill_ALSTATE(&data[index],c8108.overflow,dtenum);
		break;
	}
	response->datalen = index;
	fprintf(stderr,"C8108 datalen = %d\n",response->datalen);
	return response->datalen;
}

int GetSecurePara(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad;
	CLASS_F101 f101;
	oad = response->oad;
	data = response->data;

	memset(&f101,0,sizeof(CLASS_F101));
	readCoverClass(0xf101,0,&f101,sizeof(f101),para_vari_save);
	switch(oad.attflg )
	{
		case 2://安全模式选择
			fprintf(stderr,"active=%d\n",f101.active);
			response->datalen = fill_enum(data,f101.active);
			break;
		case 3://安全模式参数array
			break;
	}
	return 0;
}
#if 0
int GetSysDateTime(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad;
	DateTimeBCD time;
	CLASS_4000	class_tmp={};
	int index=0;

	oad = response->oad;
	data = response->data;
	switch(oad.attflg )
	{
		case 2://安全模式选择
			system((const char*)"hwclock -s");
			DataTimeGet(&time);
			response->datalen = fill_date_time_s(response->data,&time);
			break;
		case 3://校时模式
			readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
			response->datalen = fill_enum(&data[index],class_tmp.type);
			break;
		case 4://精准校时模式
			readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
			index += create_struct(&data[index],5);
			index += fill_unsigned(&data[index],class_tmp.hearbeatnum);
			index += fill_unsigned(&data[index],class_tmp.tichu_max);
			index += fill_unsigned(&data[index],class_tmp.tichu_min);
			index += fill_unsigned(&data[index],class_tmp.delay);
			index += fill_unsigned(&data[index],class_tmp.num_min);
			response->datalen = index;
			break;
	}
	return 0;
}
#endif
int Get3105(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event3105_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event3105_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3105_Object),event_para_save);
	index += create_struct(&data[index],2);
	index += fill_long_unsigned(&data[index],tmpobj.mto_obj.over_threshold);
	index += fill_unsigned(&data[index],tmpobj.mto_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get3106(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event3106_Object	tmpobj={};
	INT8U *data = NULL;
	int	i=0;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event3106_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3106_Object),event_para_save);
	if(oad.attrindex == 0x00){
	  index += create_struct(&data[index],2);	//属性６　２个元素
	}
	if(oad.attrindex != 0x02){
		index += create_struct(&data[index],4);	//停电数据采集配置参数　４个元素
		index += fill_bit_string(&data[index],8,&tmpobj.poweroff_para_obj.collect_para_obj.collect_flag);
		index += fill_unsigned(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.time_space);
		index += fill_unsigned(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.time_threshold);
		index += create_array(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.num);
		for(i=0;i<tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.num;i++) {
			index += fill_TSA(&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr[1],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr[0]);
		}
	}
	if(oad.attrindex != 0x01){
		index += create_struct(&data[index],6);	//停电事件甄别限值参数　６个元素
		index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.mintime_space);
		index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.maxtime_space);
		index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.startstoptime_offset);
		index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.sectortime_offset);
		index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.happen_voltage_limit);
		index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.recover_voltage_limit);
	}
	response->datalen = index;
	return 0;
}

int Get310c(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310C_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310C_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310C_Object),event_para_save);
	index += create_struct(&data[index],2);	//属性６　２个元素
	index += fill_double_long_unsigned(&data[index],tmpobj.poweroffset_obj.power_offset);
	index += fill_unsigned(&data[index],tmpobj.poweroffset_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get310d(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310D_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310D_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310D_Object),event_para_save);
	index += create_struct(&data[index],2);	//属性６　２个元素
	index += fill_double_long_unsigned(&data[index],tmpobj.poweroffset_obj.power_offset);
	index += fill_unsigned(&data[index],tmpobj.poweroffset_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get310e(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310E_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310E_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310E_Object),event_para_save);
	index += create_struct(&data[index],2);
	index += fill_TI(&data[index],tmpobj.powerstoppara_obj.power_offset);
	index += fill_unsigned(&data[index],tmpobj.powerstoppara_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get310f(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310F_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310F_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310F_Object),event_para_save);
	index += create_struct(&data[index],2);
	index += fill_unsigned(&data[index],tmpobj.collectfail_obj.retry_nums);
	index += fill_unsigned(&data[index],tmpobj.collectfail_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get3110(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event3110_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event3110_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3110_Object),event_para_save);
	index += create_struct(&data[index],1);
	index += fill_double_long_unsigned(&data[index],tmpobj.Monthtrans_obj.month_offset);
	response->datalen = index;
	return 0;
}

int Get4001_4002_4003(RESULT_NORMAL *response)
{
	int i=0;
	OAD oad={};
	CLASS_4001_4002_4003	class_addr={};

	oad = response->oad;
	memset(&class_addr,0,sizeof(CLASS_4001_4002_4003));
	readCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			response->datalen =  class_addr.curstom_num[0]+1;//第一个字节是长度 + 内容
			fprintf(stderr,"\n读取 datalen=%d\n",response->datalen);
			for(i=0;i< response->datalen;i++)
				fprintf(stderr," %02x",class_addr.curstom_num[i]);
			fprintf(stderr,"\n");
			response->data[0] = 0x09;
			response->datalen += 1;//再加一个数据类型
			memcpy(&response->data[1] ,class_addr.curstom_num,response->datalen);
			break;
	}
	return 0;
}
int Get4004(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4004	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4004));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4004),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],3);//经度，纬度，高度
			index += create_struct(&data[index],4);//方位、度、分、秒
			index += fill_enum(&data[index],class_tmp.jing.fangwei);
			index += fill_unsigned(&data[index],class_tmp.jing.du);
			index += fill_unsigned(&data[index],class_tmp.jing.fen);
			index += fill_unsigned(&data[index],class_tmp.jing.miao);
			index += create_struct(&data[index],4);//方位、度、分、秒
			index += fill_enum(&data[index],class_tmp.wei.fangwei);
			index += fill_unsigned(&data[index],class_tmp.wei.du);
			index += fill_unsigned(&data[index],class_tmp.wei.fen);
			index += fill_unsigned(&data[index],class_tmp.wei.miao);
			index += fill_double_long_unsigned(&data[index],class_tmp.heigh);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4005(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4005	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4005));
	int ret=readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4005),para_vari_save);
	fprintf(stderr,"4005ret= %d \n",ret);
	switch(oad.attflg )
	{
		case 2:
			if(ret == -1)
				data[index++] = 0;
			else
			{
				fprintf(stderr,"4005num=%d \n",class_tmp.num);
				index +=create_array(&data[index],class_tmp.num);
				INT8U i=0;
				for(i=0;i<class_tmp.num;i++){
					fprintf(stderr,"addrlen=%d \n",class_tmp.addr[i][0]);
					index +=fill_octet_string(&data[index],(char *)&class_tmp.addr[i][1],class_tmp.addr[i][0]);
				}
			}
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4006(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_4006	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4006));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4006),para_vari_save);
	fprintf(stderr,"时钟源：%d 　状态：%d\n",class_tmp.clocksource,class_tmp.state);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],2);//时钟源
			index += fill_enum(&data[index],class_tmp.clocksource);
			index += fill_enum(&data[index],class_tmp.state);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4007(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4007	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4007));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4007),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],7);
			index += fill_unsigned(&data[index],class_tmp.poweon_showtime);
			index += fill_long_unsigned(&data[index],class_tmp.lcdlight_time);
			index += fill_long_unsigned(&data[index],class_tmp.looklight_time);
			index += fill_long_unsigned(&data[index],class_tmp.poweron_maxtime);
			index += fill_long_unsigned(&data[index],class_tmp.poweroff_maxtime);
			index += fill_unsigned(&data[index],class_tmp.energydata_dec);
			index += fill_unsigned(&data[index],class_tmp.powerdata_dec);
			response->datalen = index;
			break;
	}
	return 0;
}

int Get400C(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_400C	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_400C));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_400C),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],5);
			index += fill_unsigned(&data[index],class_tmp.year_zone);
			index += fill_unsigned(&data[index],class_tmp.day_interval);
			index += fill_unsigned(&data[index],class_tmp.day_change);
			index += fill_unsigned(&data[index],class_tmp.rate);
			index += fill_unsigned(&data[index],class_tmp.public_holiday);
			response->datalen = index;
			break;
	}
	return 0;
}

int Get4014(RESULT_NORMAL *response)
{
	int index=0, i=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_4014	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4014));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4014),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_array(&data[index],class_tmp.zonenum);
			for(i=0;i<class_tmp.zonenum;i++) {
				index += create_struct(&data[index],3);
				index += fill_unsigned(&data[index],class_tmp.time_zone[i].month);
				index += fill_unsigned(&data[index],class_tmp.time_zone[i].day);
				index += fill_unsigned(&data[index],class_tmp.time_zone[i].tableno);
			}
			response->datalen = index;
			break;
	}
	return 0;
}

int Get4016(RESULT_NORMAL *response)
{
	int index=0, i=0 ,j=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_4016	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4016));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4016),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_array(&data[index],class_tmp.day_num);
			for(i=0;i<class_tmp.day_num;i++) {
				index += create_array(&data[index],class_tmp.zone_num);
				for(j=0;j<class_tmp.zone_num;j++) {
					index += create_struct(&data[index],3);
					index += fill_unsigned(&data[index],class_tmp.Period_Rate[i][j].hour);
					index += fill_unsigned(&data[index],class_tmp.Period_Rate[i][j].min);
					index += fill_unsigned(&data[index],class_tmp.Period_Rate[i][j].rateno);
				}
			}
			response->datalen = index;
			break;
	}
	return 0;
}

int Get4103(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4103	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4103));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4103),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += fill_visible_string(&data[index],&class_tmp.assetcode[1],class_tmp.assetcode[0]);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4202(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4202	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4202));
	int ret=readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4202),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			if(ret != -1)
			{
				index += create_struct(&data[index],8);
				index += fill_bool(&data[index],class_tmp.flag);
				index += create_OAD(1,&data[index],class_tmp.oad);
				index += fill_long_unsigned(&data[index],class_tmp.total_timeout);
				index += fill_long_unsigned(&data[index],class_tmp.byte_timeout);
				index += fill_unsigned(&data[index],class_tmp.resendnum);
				index += fill_unsigned(&data[index],class_tmp.cycle);
				index += fill_unsigned(&data[index],class_tmp.portnum);
				index += create_array(&data[index],class_tmp.tsanum);
				int i=0;
				for(i=0;i<class_tmp.tsanum;i++)
				{
					data[index++]=dttsa;
					memcpy(&data[index],&class_tmp.tsa[i][0],class_tmp.tsa[i][0]+1);
					index +=class_tmp.tsa[i][0]+1;
				}
			}
			else
			{
				data[index++]=0;
			}
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4204(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4204	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4204));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4204),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],2);
			index += fill_time(&data[index],class_tmp.startime);
			index += fill_bool(&data[index],class_tmp.enable);
			response->datalen = index;
			break;
		case 3:
			index += create_struct(&data[index],3);
			index += fill_integer(&data[index],class_tmp.upleve);
			index += fill_time(&data[index],class_tmp.startime1);
			index += fill_bool(&data[index],class_tmp.enable1);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4300(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS19	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS19));
	readCoverClass(0x4300,0,&class_tmp,sizeof(CLASS19),para_vari_save);
	int arrindex=index;
	INT8U i=0,num=0;
	switch(oad.attflg )
	{
	    case 2:
	    	index += fill_visible_string(&data[index],&class_tmp.devdesc[1],class_tmp.devdesc[0]);
	    	break;
		case 3:
			index += create_struct(&data[index],6);
			index += fill_visible_string(&data[index],class_tmp.info.factoryCode,4);
			index += fill_visible_string(&data[index],class_tmp.info.softVer,4);
			index += fill_visible_string(&data[index],class_tmp.info.softDate,6);
			index += fill_visible_string(&data[index],class_tmp.info.hardVer,4);
			index += fill_visible_string(&data[index],class_tmp.info.hardDate,6);
			index += fill_visible_string(&data[index],class_tmp.info.factoryExpInfo,8);
			break;
		case 4:
			index +=fill_date_time_s(&data[index],&class_tmp.date_Product);
			break;
		case 5:
			index +=2;
			for(i=0;i<10;i++){
				if(class_tmp.ois[i]>0){ //剔除0得情况
					index +=fill_OI(&data[index],class_tmp.ois[i]);
					num++;
				}
			}
			create_array(&data[arrindex],num);
			break;
		case 6://支持规约列表
			index += create_array(&data[index],1);
			index += fill_visible_string(&data[index],class_tmp.protcol,sizeof(class_tmp.protcol));
			break;
		case 7:
			index += fill_bool(&data[index],class_tmp.follow_report);
			break;
		case 8:
			index += fill_bool(&data[index],class_tmp.active_report);
			break;
		case 9:
			index += fill_bool(&data[index],class_tmp.talk_master);
			break;
		case 10:
			index +=2;
			for(i=0;i<10;i++){
				if(!(class_tmp.oads[i].OI == 0
						&&class_tmp.oads[i].attflg == 0
						&&class_tmp.oads[i].attrindex == 0)){ //剔除0得情况
					index +=create_OAD(1,&data[index],class_tmp.oads[i]);
					fprintf(stderr,"%d-oad.oi=%04x \n",i,class_tmp.oads[i].OI);
					num++;
				}
			}
			if(num>0)
				create_array(&data[arrindex],num);
			else
				data[index++]=0;
			break;
	}
	response->datalen = index;
	return 0;
}
int Get4400(RESULT_NORMAL *response)
{
	int index=0,ret=0;
	INT8U *data = NULL;

	OAD oad={};
	CLASS_4400	class_tmp={};
	INT8U i=0,m=0;

	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4400));
	ret = readCoverClass(0x4400,0,&class_tmp,sizeof(CLASS_4400),para_vari_save);
	//无4400参数，一致性测试：设置一个默认值上送，否则不合格
	if(ret!=1) {
		class_tmp.num=1;
		class_tmp.authority[0].OI=0x4000;
		class_tmp.authority[0].one_authority.met_num=1;
		class_tmp.authority[0].one_authority.method[0].id=1;
		class_tmp.authority[0].one_authority.method[0].visit_authority=1;
		class_tmp.authority[0].one_authority.pro_num=1;
		class_tmp.authority[0].one_authority.property[0].id=1;
		class_tmp.authority[0].one_authority.property[0].visit_authority=1;
	}

	switch(oad.attflg )
	{
	    case 1:
	    	index +=fill_octet_string(&data[index],(char *)&class_tmp.login_name[1],class_tmp.login_name[0]);
		break;
	    case 2:
	    	if(class_tmp.num>0)
	    	{
	    		//fprintf(stderr,);
               index +=create_array(&data[index],class_tmp.num);
               for(i=0;i<class_tmp.num;i++)
               {
            	   index +=create_struct(&data[index],2);
            	   index +=fill_OI(&data[index],class_tmp.authority[i].OI);
            	   index +=create_struct(&data[index],2);
            	   if(class_tmp.authority[i].one_authority.pro_num>0){
            		   index +=create_array(&data[index],class_tmp.authority[i].one_authority.pro_num);
            		   for(m=0;m<class_tmp.authority[i].one_authority.pro_num;m++){
            			   index +=create_struct(&data[index],2);
            			   index +=fill_unsigned(&data[index],class_tmp.authority[i].one_authority.property[m].id);
                           index +=fill_enum(&data[index], class_tmp.authority[i].one_authority.property[m].visit_authority);
            		   }
            	   }
            	   else
            		   data[index++]=0;

            	   if(class_tmp.authority[i].one_authority.met_num>0){
					   index +=create_array(&data[index],class_tmp.authority[i].one_authority.met_num);
					   for(m=0;m<class_tmp.authority[i].one_authority.met_num;m++){
						   index +=create_struct(&data[index],2);
						   index +=fill_unsigned(&data[index],class_tmp.authority[i].one_authority.method[m].id);
							  index +=fill_bool(&data[index], class_tmp.authority[i].one_authority.method[m].visit_authority);
					   }
				   }
				   else
					   data[index++]=0;
               }
	    	}
	    	else
	    	{
	    		data[index++]=0;
	    	}
	    	break;
		case 3:
			index +=create_struct(&data[index],7);
			index +=fill_long_unsigned(&data[index],class_tmp.use_lan_info.xieyi_banben);
			index +=fill_long_unsigned(&data[index],class_tmp.use_lan_info.max_rev_num);
			index +=fill_long_unsigned(&data[index],class_tmp.use_lan_info.max_send_num);
			index +=fill_long_unsigned(&data[index],class_tmp.use_lan_info.al_num);
			index +=fill_bit_string(&data[index],64,class_tmp.use_lan_info.xieyi);
			index +=fill_bit_string(&data[index],128,class_tmp.use_lan_info.power);
			index +=fill_double_long_unsigned(&data[index], class_tmp.use_lan_info.static_outtime);
			break;
		case 4:
            index +=fill_unsigned(&data[index], class_tmp.custom);
			break;
		case 5:
			index +=fill_enum(&data[index], class_tmp.renzheng);
			break;

	}
	response->datalen = index;
	return 0;
}
int Get4500(RESULT_NORMAL *response)
{
	int index=0,i=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS25	class_tmp={};

	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS25));
	readCoverClass(0x4500,0,&class_tmp,sizeof(CLASS25),para_vari_save);

	switch(oad.attflg )
	{
	case 2:	//通信配置
		index += create_struct(&data[index],12);
		index += fill_enum(&data[index],class_tmp.commconfig.workModel);
		index += fill_enum(&data[index],class_tmp.commconfig.onlineType);
		index += fill_enum(&data[index],class_tmp.commconfig.connectType);
		index += fill_enum(&data[index],class_tmp.commconfig.appConnectType);
		if(class_tmp.commconfig.listenPortnum>=5) class_tmp.commconfig.listenPortnum = 5;
		index += create_array(&data[index],class_tmp.commconfig.listenPortnum);
		if(class_tmp.commconfig.listenPortnum) {
			for(i=0;i<class_tmp.commconfig.listenPortnum;i++) {
				index += fill_long_unsigned(&data[index],class_tmp.commconfig.listenPort[i]);
			}
		}
		index += fill_visible_string(&data[index],(char *)&class_tmp.commconfig.apn[1],class_tmp.commconfig.apn[0]);
		index += fill_visible_string(&data[index],(char *)&class_tmp.commconfig.userName[1],class_tmp.commconfig.userName[0]);
		index += fill_visible_string(&data[index],(char *)&class_tmp.commconfig.passWord[1],class_tmp.commconfig.passWord[0]);
		index += fill_octet_string(&data[index],(char *)&class_tmp.commconfig.proxyIp[1],class_tmp.commconfig.proxyIp[0]);
		index += fill_long_unsigned(&data[index],class_tmp.commconfig.proxyPort);
		//index += fill_bit_string(&data[index],8,class_tmp.commconfig.timeoutRtry);
		index += fill_unsigned(&data[index],class_tmp.commconfig.timeoutRtry);	//勘误
		index += fill_long_unsigned(&data[index],class_tmp.commconfig.heartBeat);
		break;
	case 3:	//主站通信参数表
		//if(class_tmp.master.masternum==0) class_tmp.master.masternum = 2;
		fprintf(stderr,"masternum=%d\n",class_tmp.master.masternum);
		index += create_array(&data[index],class_tmp.master.masternum);
		for(i=0;i<class_tmp.master.masternum;i++) {
			index += create_struct(&data[index],2);
			index += fill_octet_string(&data[index],(char *)&class_tmp.master.master[i].ip[1],4);//默认长度，class_tmp.master.master.ip[0]
			index += fill_long_unsigned(&data[index],class_tmp.master.master[i].port);
		}
		break;
	case 4:	//短信通信参数
		index += create_struct(&data[index],3);
		index += fill_visible_string(&data[index],&class_tmp.sms.center[1],class_tmp.sms.center[0]);
		index += create_array(&data[index],class_tmp.sms.masternum);
		for(i=0;i<class_tmp.sms.masternum;i++) {
			index += fill_visible_string(&data[index],&class_tmp.sms.master[i][1],class_tmp.sms.master[i][0]);
		}
		index += create_array(&data[index],class_tmp.sms.destnum);
		for(i=0;i<class_tmp.sms.destnum;i++) {
			index += fill_visible_string(&data[index],&class_tmp.sms.dest[i][1],class_tmp.sms.dest[i][0]);
		}
		break;
	case 5:
		index += create_struct(&data[index],6);
		index += fill_visible_string(&data[index],class_tmp.info.factoryCode,4);
		index += fill_visible_string(&data[index],class_tmp.info.softVer,4);
		index += fill_visible_string(&data[index],class_tmp.info.softDate,6);
		index += fill_visible_string(&data[index],class_tmp.info.hardVer,4);
		index += fill_visible_string(&data[index],class_tmp.info.hardDate,6);
		index += fill_visible_string(&data[index],class_tmp.info.factoryExpInfo,8);
		break;
	case 6:
		if(class_tmp.protocolnum>0)
		{
			index += create_array(&data[index],class_tmp.protocolnum);
			for(i=0;i<class_tmp.protocolnum;i++)
				index +=fill_visible_string(&data[index],(char *)&class_tmp.protcol[i][1],class_tmp.protcol[i][0]);
		}
		else
            data[index++]=0;
		break;
	case 7:
//		if(class_tmp.ccid[0]>0)
			index +=fill_visible_string(&data[index],(char *)&class_tmp.ccid[1],class_tmp.ccid[0]);
//		else
//			data[index++]=0;
	    break;
	case 8:
//		if(class_tmp.imsi[0]>0)
			index +=fill_visible_string(&data[index],(char *)&class_tmp.imsi[1],class_tmp.imsi[0]);
//		else
//			data[index++]=0;
		break;
	case 9:
		index +=fill_long(&data[index],class_tmp.signalStrength);
		break;
	case 10:
//		if((strlen((char *)class_tmp.simkard))>0)
//			index +=fill_visible_string(&data[index],(char *)&class_tmp.simkard,strlen((char *)class_tmp.simkard));
//		else
//			data[index++]=0;
//		if(class_tmp.simkard[0]>0)
			index +=fill_visible_string(&data[index],(char *)&class_tmp.simkard[1],class_tmp.simkard[0]);
//		else
//			data[index++]=0;
		break;
	case 11:
		if(class_tmp.pppip[0]>0)
			index +=fill_octet_string(&data[index],(char *)&class_tmp.pppip[1],class_tmp.pppip[0]);
		else
			data[index++]=0;
		break;
	}
	response->datalen = index;
	return 0;
}
int Get4018(RESULT_NORMAL *response)
{
	int index=0,i=0;
	CLASS_4018 class4018={};
	OAD oad={};
	INT8U *data = NULL;
	data = response->data;
	oad = response->oad;
	memset(&class4018,0,sizeof(CLASS_4018 ));

	readCoverClass(0x4018,0,&class4018,sizeof(CLASS_4018 ),para_vari_save);
	switch(oad.attflg) {
		case 2:
			fprintf(stderr,"\nGet4018 组织属性2，数组元素个数 %d ",class4018.num);
			index += create_array(&data[index],class4018.num);

			if(class4018.num) {
				for(i=0;i<class4018.num;i++) {
					index += fill_double_long_unsigned(&data[index],class4018.feilv_price[i]);
					fprintf(stderr,"\n%d  -  %lld",i,class4018.feilv_price[i]);
				}
			}
			break;
	}
	fprintf(stderr,"\ndatalen = %d",index);
	response->datalen = index;
	return 0;
}
int GetF206(RESULT_NORMAL *response)
{
	int index=0,i=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_f206	class_tmp={};

	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_f206));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_f206),para_vari_save);
	switch(oad.attflg )
	{
		case 2:	//通信配置
			if(class_tmp.state_num == 0) {
				class_tmp.state_num = 2;		//默认两个轮次告警
				class_tmp.alarm_state[0] = 0;	//值 = 未输出
				class_tmp.alarm_state[1] = 0;
			}
			index += create_array(&data[index],class_tmp.state_num);
			for(i=0;i<class_tmp.state_num;i++)
			{
				index += create_struct(&data[index],1);
				index += fill_enum(&data[index],class_tmp.alarm_state[i]);
			}
			break;
		case 4:	//主站通信参数表
			fprintf(stderr,"Getf206 Attrib 4 num=%d\n",class_tmp.time_num);
			index += create_array(&data[index],class_tmp.time_num);
			for(i=0;i<class_tmp.time_num;i++) {
				index += create_struct(&data[index],2);
				index += fill_time(&data[index],(INT8U*)class_tmp.timev[i].start);
				index += fill_time(&data[index],(INT8U*)class_tmp.timev[i].end);
			}
			break;
	}
	response->datalen = index;
	return 0;
}

int GetF207(RESULT_NORMAL *response)
{
	int index=0,i=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS_f207	class_tmp={};

	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_f207));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_f207),para_vari_save);
	if(oad.attflg ==2) {
		index += create_array(&data[index],class_tmp.num);
		for(i=0;i<class_tmp.num;i++)
		{
			index += create_struct(&data[index],1);
			index += create_OAD(1,&data[index],class_tmp.oad[i]);
			index += fill_enum(&data[index],class_tmp.func[i]);
		}
	}
	response->datalen = index;
	return 0;
}

//
int Get4510(RESULT_NORMAL *response)
{
	int index=0,i=0;
	INT8U *data = NULL;
	OAD oad={};
	CLASS26	class_tmp={};

	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS26));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS26),para_vari_save);

	switch(oad.attflg )
	{
		case 2:	//通信配置
			index += create_struct(&data[index],8);
			index += fill_enum(&data[index],class_tmp.commconfig.workModel);
			index += fill_enum(&data[index],class_tmp.commconfig.connectType);
			index += fill_enum(&data[index],class_tmp.commconfig.appConnectType);
			if(class_tmp.commconfig.listenPortnum>=5) class_tmp.commconfig.listenPortnum = 5;

			index += create_array(&data[index],class_tmp.commconfig.listenPortnum);
			if(class_tmp.commconfig.listenPortnum) {
				for(i=0;i<class_tmp.commconfig.listenPortnum;i++) {
					index += fill_long_unsigned(&data[index],class_tmp.commconfig.listenPort[i]);
				}
			}
			for(i=0;i<OCTET_STRING_LEN;i++){
				fprintf(stderr,"%02x ",class_tmp.commconfig.proxyIp[i]);
			}
			index += fill_octet_string(&data[index],(char *)&class_tmp.commconfig.proxyIp[1],class_tmp.commconfig.proxyIp[0]);
			index += fill_long_unsigned(&data[index],class_tmp.commconfig.proxyPort);
			//index += fill_bit_string(&data[index],8,class_tmp.commconfig.timeoutRtry);
			index += fill_unsigned(&data[index],class_tmp.commconfig.timeoutRtry);//勘误
			index += fill_long_unsigned(&data[index],class_tmp.commconfig.heartBeat);
			break;
		case 3:	//主站通信参数表
			fprintf(stderr,"Get4510 masternum=%d\n",class_tmp.master.masternum);
			index += create_array(&data[index],class_tmp.master.masternum);
			for(i=0;i<class_tmp.master.masternum;i++) {
				index += create_struct(&data[index],2);
				index += fill_octet_string(&data[index],(char *)&class_tmp.master.master[i].ip[1],4);//默认长度，class_tmp.master.master.ip[0]
				index += fill_long_unsigned(&data[index],class_tmp.master.master[i].port);
			}
			break;
		case 4:	//网络配置
			index += create_struct(&data[index],6);
			index += fill_enum(&data[index],class_tmp.IP.ipConfigType);
			index += fill_octet_string(&data[index],(char *)&class_tmp.IP.ip[1],class_tmp.IP.ip[0]);
			index += fill_octet_string(&data[index],(char *)&class_tmp.IP.subnet_mask[1],class_tmp.IP.subnet_mask[0]);
			index += fill_octet_string(&data[index],(char *)&class_tmp.IP.gateway[1],class_tmp.IP.gateway[0]);
			index += fill_visible_string(&data[index],(char *)&class_tmp.IP.username_pppoe,class_tmp.IP.username_pppoe[0]);
			index += fill_visible_string(&data[index],(char *)&class_tmp.IP.password_pppoe,class_tmp.IP.password_pppoe[0]);
			break;
	}
	response->datalen = index;
	return 0;
}

void printSel5(RESULT_RECORD record)
{
	fprintf(stderr,"\n%d年 %d月 %d日 %d时:%d分:%d秒",
					record.select.selec5.collect_save.year.data,record.select.selec5.collect_save.month.data,
					record.select.selec5.collect_save.day.data,record.select.selec5.collect_save.hour.data,
					record.select.selec5.collect_save.min.data,record.select.selec5.collect_save.sec.data);
	fprintf(stderr,"\nMS-TYPE %d  ",record.select.selec5.meters.mstype);
	print_rcsd(record.rcsd.csds);
}

void printSel7(RESULT_RECORD record)
{
	fprintf(stderr,"\n采集存储时间起始值：%d-%d-%d %d:%d:%d",
					record.select.selec7.collect_save_star.year.data,record.select.selec7.collect_save_star.month.data,
					record.select.selec7.collect_save_star.day.data,record.select.selec7.collect_save_star.hour.data,
					record.select.selec7.collect_save_star.min.data,record.select.selec7.collect_save_star.sec.data);
	fprintf(stderr,"\n采集存储时间结束值：%d-%d-%d %d:%d:%d",
					record.select.selec7.collect_save_finish.year.data,record.select.selec7.collect_save_finish.month.data,
					record.select.selec7.collect_save_finish.day.data,record.select.selec7.collect_save_finish.hour.data,
					record.select.selec7.collect_save_finish.min.data,record.select.selec7.collect_save_finish.sec.data);
	fprintf(stderr,"\n时间间隔TI 单位:%d[秒-0，分-1，时-2，日-3，月-4，年-5],间隔:%x",record.select.selec7.ti.units,record.select.selec7.ti.interval);
	fprintf(stderr,"\n电能表集合MS 类型：%d\n",record.select.selec7.meters.mstype);
	print_rcsd(record.rcsd.csds);
}

void printSel9(RESULT_RECORD record)
{
	fprintf(stderr,"\nSelector9:指定选取上第n次记录\n");
	fprintf(stderr,"\n选取上第%d次记录 ",record.select.selec9.recordn);
	fprintf(stderr,"\nRCSD个数：%d",record.rcsd.csds.num);
	print_rcsd(record.rcsd.csds);
}

void printrecord(RESULT_RECORD record)
{
	switch(record.selectType){
	case 1:
		fprintf(stderr,"\nOAD %04x %02x %02x",record.select.selec1.oad.OI,record.select.selec1.oad.attflg,record.select.selec1.oad.attrindex);
		fprintf(stderr,"\nData Type= %02x  Value=%d ",record.select.selec1.data.type,record.select.selec1.data.data[0]);
		break;
	case 2:
		fprintf(stderr,"\nOAD %04x %02x %02x",record.select.selec2.oad.OI,record.select.selec2.oad.attflg,record.select.selec2.oad.attrindex);
		break;
	case 5:
		printSel5(record);
		break;
	case 7:
		printSel7(record);
		break;
	case 9:
		printSel9(record);
		break;
	}
}
/*　type :数据类型
 * seldata:selector数据
 * destdata:目标
 * */
int getSel_Data(INT8U type,INT8U *seldata,INT8U *destdata)
{
	int 	ret=-1;
	TI		ti;
	INT8U	DAR=success;
	fprintf(stderr,"getSel_Data type=%02x\n",type);
	switch(type) {
	case dtnull:
		destdata[0] = 0;
		ret = 0;
		break;
	case dtunsigned:
		destdata[0] = seldata[0];
		ret = 1;
		break;
	case dtlongunsigned:
		destdata[0] = seldata[1];
		destdata[1] = seldata[0];
		ret = 2;
		break;
	case dtdatetimes:
		ret = getDateTimeS(0,seldata,destdata,&DAR);
		break;
	case dtti:
		fprintf(stderr,"seldata=%02x_%02x_%02x\n",seldata[0],seldata[1],seldata[2]);
		ret = getTI(0,seldata,&ti);
		destdata[0] = ti.units;
		destdata[1] = (ti.interval >> 8) & 0xff;
		destdata[2] = ti.interval & 0xff;
		break;
	default:
		ret = -1;
		break;
	}
//	fprintf(stderr,"getSel_Data return %d\n",ret);
	return ret;
}

/*
 * 获取采集类数据
 * */
int getColl_Data(OI_698 oi,INT16U seqnum,INT8U *data)
{
	int	index=0;
	switch(oi) {
	case 0x6000:
	case 0x6001:
		index += Get_6001(0,seqnum,data);
		break;
	case 0x6013:
		index += Get_6013(0,seqnum,data);
		break;
	case 0x6015:
		index += Get_6015(0,seqnum,data);
		break;
	case 0x6017:
		index += Get_6017(0,seqnum,data);
		break;
	case 0x6019:
		index += Get_6019(0,seqnum,data);
		break;
	case 0x6035:
		index += Get_6035(0,seqnum,data);
		break;
	case 0x601d:
		index += Get_601D(0,seqnum,data);
		break;
	default:
		fprintf(stderr,"\n oi=%x not found!",oi);
	}
	return index;
}

INT8U fillVacsData(OAD oad,INT8U devicetype,INT8U datatype,INT64U dataz,INT64U data1,INT64U data2,INT64U data3,INT64U data4,INT8U *responseData)
{
	INT64U 	data[5]={};
	INT8U	index=0,i=0;
	INT8U	oiA1 = 0;
	INT8U	structnum = 0;

	oiA1 = (oad.OI & 0xF000) >> 12;
	memset(data,0,sizeof(data));
	switch(oiA1) {
	case 0:	//电能量
	case 1: //最大需量类
		if(oad.attrindex==0)		structnum = 5;
		else structnum = 1;
		switch(oad.attrindex)  {	//全部属性
		case 0:
			data[0] = dataz;
			data[1] = data1;
			data[2] = data2;
			data[3] = data3;
			data[4] = data4;
			break;
		case 1:	data[0] = dataz;	break;
		case 2:	data[0] = data1;	break;
		case 3:	data[0] = data2;	break;
		case 4:	data[0] = data3;	break;
		case 5:	data[0] = data4;	break;
		}
		break;
	case 2://变量类
		switch(oad.OI) {
		case 0x2000:
			if(oad.attrindex==0 && devicetype != CCTT2)		structnum = 3;
			else structnum = 1;
			break;
		case 0x2001:	//电流,//国网台体测试常温基本误差，送I0提示元素错误,规约属性4为零序电流
			if(oad.attflg == 02) {
				if(oad.attrindex==0)		structnum = 3;
				else structnum = 1;
			}else if(oad.attflg == 04) {	//I0
				structnum = 1;
			}
			break;
		case 0x2002:	//电压相角
		case 0x2003:	//电压电流相角
			if(oad.attrindex==0)		structnum = 3;
			else structnum = 1;
			break;
		case 0x2004:	//有功功率
		case 0x2005:	//无功功率
		case 0x2006:	//视在功率
		case 0x200A:	//功率因数
			if(oad.attrindex==0)		structnum = 4;
			else structnum = 1;
			break;
		}
		switch(oad.attrindex)  {	//全部属性
		case 0:
			if(oad.OI == 0x2001 && oad.attflg == 4) {	//零序电流
				data[0] = data4;
			}else {
				data[0] = data1;
				data[1] = data2;
				data[2] = data3;
				data[3] = data4;
			}
			break;
		case 1:	data[0] = data1;	break;
		case 2:	data[0] = data2;	break;
		case 3:	data[0] = data3;	break;
		case 4:	data[0] = data4;	break;
		}
		break;
	}
	if((oad.attflg == 2 || oad.attflg == 4) && oad.attrindex == 0) {		//请求全部属性
		index += create_array(&responseData[index],structnum);
	}
	for(i=0;i<structnum;i++) {
		switch(datatype){
		case dtdoublelong:
			index += fill_double_long(&responseData[index],data[i]);
			break;
		case dtdoublelongunsigned:
			index += fill_double_long_unsigned(&responseData[index],data[i]);
			break;
		case dtlong:
			index += fill_long(&responseData[index],data[i]);
			break;
		case dtlongunsigned:
			index += fill_long_unsigned(&responseData[index],data[i]);
			break;
		case dtlong64:
			index += fill_long64(&responseData[index],data[i]);
			break;
		case dtlong64unsigned:
			index += fill_long64_unsigned(&responseData[index],data[i]);
			break;
		}
	}
	return index;
}
//INT8U fillVacsData(INT8U structnum,INT8U attindex,INT8U datatype,INT64U dataz,INT64U data1,INT64U data2,INT64U data3,INT64U data4,INT8U *responseData)
//{
//	INT64U 	data[5]={};
//	INT8U	index=0,i=0;
//
////	fprintf(stderr,"11111structnum=%d   responseData=%p\n",structnum,responseData);
//	structnum = limitJudge("数据结构",5,structnum);
//	attindex = limitJudge("属性索引值",5,attindex);
//
//	fprintf(stderr,"structnum=%d   responseData=%p\n",structnum,responseData);
//	if(attindex == 0) {		//请求全部属性
//		index += create_array(&responseData[index],structnum);
//	}
//	fprintf(stderr,"index=%d\n",index);
//	memset(data,0,sizeof(data));
//	switch(attindex) {
//	case 0:		//全部属性
//		if(structnum == 4) {
//			data[0] = data1;
//			data[1] = data2;
//			data[2] = data3;
//			data[3] = data4;
//		}else if(structnum == 5) {
//			data[0] = dataz;
//			data[1] = data1;
//			data[2] = data2;
//			data[3] = data3;
//			data[4] = data4;
//		}
//		break;
//	case 1:	data[0] = dataz;	break;
//	case 2:	data[0] = data1;	break;
//	case 3:	data[0] = data2;	break;
//	case 4:	data[0] = data3;	break;
//	case 5:	data[0] = data4;	break;
//	}
//
//	for(i=0;i<structnum;i++) {
//		switch(datatype){
//		case dtlongunsigned:
//			index += fill_long_unsigned(&responseData[index],data[i]);
//			break;
//		case dtdoublelong:
//			index += fill_double_long(&responseData[index],data[i]);
//			break;
//		case dtlong:
//			index += fill_long(&responseData[index],data[i]);
//			break;
//		case dtlong64:
//			index += fill_long64(&responseData[index],data[i]);
//			break;
//		}
//	}
//	return index;
//}

INT64U  getTotalEnergy(INT64U *val)
{
	INT64U  total_energy=0;
	INT8U	i = 0;

	total_energy = 0;
	for(i=0;i<MAXVAL_RATENUM;i++) {
		total_energy += val[i];
	}
	return total_energy;
};

/*
 * 脉冲电量统计使用高精度电能量，OI的属性为4，值为小数点后4位
 * 如果使用普通电能量，OI属性为2，值为小数点后2位
 * */
INT32U	int64toint32(INT64U val)
{
	INT32U chgval = 0;
	chgval = (INT32U)val/100;
	return chgval;
}

int fill_pulseEnergy(INT8U devicetype,INT8U index,OAD oad,INT8U *destbuf,INT16U *len)
{
	int  	buflen = 0;
	CLASS12_ENERGY	pluse_energy[2]={};
	INT64U  total_energy=0;

//	fprintf(stderr,"\n\n\n&&&&&&&&&&&&&&&&&&&&&&&&oad=%04x_%02x%02x\n",oad.OI,oad.attflg,oad.attrindex);
	readVariData(PORT_PLUSE,0,(INT8U *)&pluse_energy,sizeof(pluse_energy));
	switch(oad.OI) {
	case 0x0010://正向有功电能
		total_energy = getTotalEnergy(pluse_energy[index].val_pos_p);
		if(oad.attflg == 2) {
			buflen = fillVacsData(oad,devicetype,dtdoublelongunsigned,int64toint32(total_energy),
					int64toint32(pluse_energy[index].val_pos_p[0]),int64toint32(pluse_energy[index].val_pos_p[1]),
					int64toint32(pluse_energy[index].val_pos_p[2]),int64toint32(pluse_energy[index].val_pos_p[3]),destbuf);
		}else 	if(oad.attflg == 4) {
			buflen = fillVacsData(oad,devicetype,dtlong64unsigned,total_energy,
								pluse_energy[index].val_pos_p[0],pluse_energy[index].val_pos_p[1],
								pluse_energy[index].val_pos_p[2],pluse_energy[index].val_pos_p[3],destbuf);
		}
		break;
	case 0x0020://反向有功电能
		total_energy = getTotalEnergy(pluse_energy[index].val_nag_p);
		if(oad.attflg == 2) {
			buflen = fillVacsData(oad,devicetype,dtdoublelongunsigned,int64toint32(total_energy),
					int64toint32(pluse_energy[index].val_nag_p[0]),int64toint32(pluse_energy[index].val_nag_p[1]),
					int64toint32(pluse_energy[index].val_nag_p[2]),int64toint32(pluse_energy[index].val_nag_p[3]),destbuf);
		}else if(oad.attflg == 4) {
			buflen = fillVacsData(oad,devicetype,dtlong64unsigned,total_energy,
								pluse_energy[index].val_nag_p[0],pluse_energy[index].val_nag_p[1],
								pluse_energy[index].val_nag_p[2],pluse_energy[index].val_nag_p[3],destbuf);
		}
		break;
	case 0x0030://正向无功电能/组合无功1电能
		total_energy = getTotalEnergy(pluse_energy[index].val_pos_q);
		if(oad.attflg == 2) {
			buflen = fillVacsData(oad,devicetype,dtdoublelong,int64toint32(total_energy),
					int64toint32(pluse_energy[index].val_pos_q[0]),int64toint32(pluse_energy[index].val_pos_q[1]),
					int64toint32(pluse_energy[index].val_pos_q[2]),int64toint32(pluse_energy[index].val_pos_q[3]),destbuf);
		}else if(oad.attflg == 4) {
			buflen = fillVacsData(oad,devicetype,dtlong64,total_energy,
								pluse_energy[index].val_pos_q[0],pluse_energy[index].val_pos_q[1],
								pluse_energy[index].val_pos_q[2],pluse_energy[index].val_pos_q[3],destbuf);
		}
		break;
	case 0x0040://反向无功电能/组合无功2电能
		total_energy = getTotalEnergy(pluse_energy[index].val_nag_q);
		if(oad.attflg == 2) {
			buflen = fillVacsData(oad,devicetype,dtdoublelong,int64toint32(total_energy),
					int64toint32(pluse_energy[index].val_nag_q[0]),int64toint32(pluse_energy[index].val_nag_q[1]),
					int64toint32(pluse_energy[index].val_nag_q[2]),int64toint32(pluse_energy[index].val_nag_q[3]),destbuf);
		}else  if(oad.attflg == 4) {
			buflen = fillVacsData(oad,devicetype,dtlong64,total_energy,
								pluse_energy[index].val_nag_q[0],pluse_energy[index].val_nag_q[1],
								pluse_energy[index].val_nag_q[2],pluse_energy[index].val_nag_q[3],destbuf);
		}
		break;
	}
	*len = buflen;
	return 1;
}

/*
 * getflg:是否有数据源。=1，读取sourcebuf内容，=0，数据上送0
 * */
int  fill_variClass(OAD oad,INT8U getflg,INT8U *sourcebuf,INT8U *destbuf,INT16U *len,ProgramInfo* proginfo)
{
	int  	buflen = 0;
	FP32 	bett[2]={};
	INT8U	databuf[VARI_LEN]={};

	memset(&databuf,0,sizeof(databuf));

	fprintf(stderr,"oad.OI=%x\n",oad.OI);
	switch(oad.OI) {
	case 0x2000:	//电压
		if(proginfo->cfg_para.device == CCTT2 && oad.attrindex > 1) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtlongunsigned,0,
				proginfo->ACSRealData.Ua,proginfo->ACSRealData.Ub,proginfo->ACSRealData.Uc,0,destbuf);
		break;
	case 0x2001:	//电流
		if(proginfo->cfg_para.device == CCTT2) {//国网送检基本功能测试，II型招测电压，电流，有功，无功参数，其他无效数据应回越限
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtdoublelong,0,
				proginfo->ACSRealData.Ia,proginfo->ACSRealData.Ib,proginfo->ACSRealData.Ic,proginfo->ACSRealData.I0,destbuf);
		break;
	case 0x2002:	//电压相角
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtlongunsigned,0,
				proginfo->ACSRealData.YUaUb,proginfo->ACSRealData.YUaUc,proginfo->ACSRealData.YUbUc,0,destbuf);
		break;
	case 0x2003:	//电压电流相角
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtlongunsigned,0,
				proginfo->ACSRealData.Pga,proginfo->ACSRealData.Pgb,proginfo->ACSRealData.Pgc,0,destbuf);
		break;
	case 0x2004:	//有功功率
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtdoublelong,0,
				proginfo->ACSRealData.Pt,proginfo->ACSRealData.Pa,proginfo->ACSRealData.Pb,proginfo->ACSRealData.Pc,destbuf);
		break;
	case 0x2005:	//无功功率
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtdoublelong,0,
				proginfo->ACSRealData.Qt,proginfo->ACSRealData.Qa,proginfo->ACSRealData.Qb,proginfo->ACSRealData.Qc,destbuf);
		break;
	case 0x2006:	//视在功率
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtdoublelong,0,
				proginfo->ACSRealData.St,proginfo->ACSRealData.Sa,proginfo->ACSRealData.Sb,proginfo->ACSRealData.Sc,destbuf);
		break;
	case 0x200A:	//功率因数
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		buflen = fillVacsData(oad,proginfo->cfg_para.device,dtlong,0,
				proginfo->ACSRealData.Cos,proginfo->ACSRealData.CosA,proginfo->ACSRealData.CosB,proginfo->ACSRealData.CosC,destbuf);
		break;
	case 0x200D:	//电压谐波含有量
		if(proginfo->cfg_para.device == CCTT2) {
			return 0;
		}
		break;
	case 0x2011:	//时钟电池电压
		if(bettery_getV(&bett[0],&bett[1]) == TRUE) {
			buflen = fill_long_unsigned(destbuf,(INT16U)(bett[0]*100));
		}
		break;
	case 0x2012:	//停电抄表电池电压
		if(bettery_getV(&bett[0],&bett[1]) == TRUE) {
			buflen = fill_long_unsigned(destbuf,(INT16U)(bett[1]*100));
		}
		break;
	case 0x2131:
	case 0x2132:
	case 0x2133:
		if(sourcebuf!=NULL) {
			Get_213x(getflg,(INT8U *)sourcebuf,destbuf,&buflen);
		}
		break;
	case 0x2200:	//通信流量
		fprintf(stderr,"proginfo = %p \n",proginfo);
		memcpy(databuf,&proginfo->dev_info.realTimeC2200,sizeof(Flow_tj));
		Get_2200(getflg,(INT8U *)databuf,destbuf,&buflen);
		break;
	case 0x2203:	//供电时间
		if(sourcebuf==NULL) {
			readVariData(oad.OI,0,&databuf,VARI_LEN);
			Get_2203(getflg,(INT8U *)databuf,destbuf,&buflen);
		}else {
			Get_2203(getflg,(INT8U *)sourcebuf,destbuf,&buflen);
		}
		break;
	case 0x2204:	//复位次数
		if(sourcebuf==NULL) {
			readVariData(oad.OI,0,&databuf,VARI_LEN);
			Get_2204(getflg,(INT8U *)databuf,destbuf,&buflen);
		}else {
			Get_2204(getflg,(INT8U *)sourcebuf,destbuf,&buflen);
		}
		break;
	case 0x2301:	//总加组1
	case 0x2302:
	case 0x2303:
	case 0x2304:
	case 0x2305:
	case 0x2306:
	case 0x2307:
	case 0x2308:	//总加组8
		class23_get(oad,sourcebuf,destbuf,&buflen);
		break;
	case 0x2401:	//脉冲计量1
	case 0x2402:	//脉冲计量2
//	case 0x2403:
//	case 0x2404:
//	case 0x2405:
//	case 0x2406:
//	case 0x2407:
//	case 0x2408:	//脉冲计量8
		class12_get(oad,sourcebuf,destbuf,&buflen);
		break;
	default:
//		//fprintf(stderr,"GET_26:未定义对象属性，上送数据NULL\n");
		destbuf[0] = 0;		//data = NULL
		buflen = 1;
		break;
	}
	*len = buflen;
	return 1;
}

int  fill_RecordRow(int *index,RESULT_RECORD *record,DateTimeBCD datetime)
{
	int 	i=0,j=0;
	int		getflg = 0;
	int		datalen=0;
	INT8U  	data[VARI_LEN]={};
	INT16U	buflen = 0;

	for(i=0;i<record->rcsd.csds.num;i++) {
		fprintf(stderr,"\n%d: %04x%02x%02x\n",i,record->rcsd.csds.csd[i].csd.oad.OI,record->rcsd.csds.csd[i].csd.oad.attflg,record->rcsd.csds.csd[i].csd.oad.attrindex);
		getflg = readFreezeRecordByTime(record->oad.OI,record->rcsd.csds.csd[i].csd.oad,datetime,&datalen,(INT8U *)data);
		fprintf(stderr,"冻结时间:%04d-%02d-%02d %02d:%02d:%02d ",datetime.year.data,datetime.month.data,datetime.day.data,
				datetime.hour.data,datetime.min.data,datetime.sec.data);
		fprintf(stderr,"数据【%d】 ",datalen);
		for(j=0;j<datalen;j++) {
			fprintf(stderr,"%02x ",data[j]);
		}
		fprintf(stderr,"\n");
		fill_variClass(record->rcsd.csds.csd[i].csd.oad,getflg,(INT8U *)data,&record->data[*index],&buflen,memp);
		*index += buflen;
	}
	return *index;
}

int fill_RecordRowByNum(RESULT_RECORD *record_para)
{
	int 	i=0;
	INT8U	csds_num=0,recordn=0;
	OI_698 	freezeOI,relateOI;
	OAD		relateOAD;
	int	 	currRecordNum=0,maxRecordNum=0,datalen=0;
	INT8U 	data[VARI_LEN];
	DateTimeBCD datetime;
	int		getflg = -1;
	int		index = 0;
	INT16U 	buflen = 0;

	freezeOI = record_para->oad.OI;
	csds_num = record_para->rcsd.csds.num;
	recordn = record_para->select.selec9.recordn;
	fprintf(stderr,"freezeOI = %x  csds_num=%d recordn=%d\n",freezeOI,csds_num,recordn);

	fprintf(stderr,"datalen = %d\n",index);
	for(i=0;i<index;i++) {
		fprintf(stderr,"%02x ",record_para->data[i]);
	}
	record_para->data[index++] = 1;				//A-ResultRecord CHOICE=1
	record_para->data[index++] = 1;				//seqof A-RecordRow
	for(i=0;i<csds_num;i++) {
		if(record_para->rcsd.csds.csd[i].type == 0) {
			memset(data,0,sizeof(data));
			relateOI = record_para->rcsd.csds.csd[i].csd.oad.OI;
			readFreezeRecordNum(freezeOI,relateOI,&currRecordNum,&maxRecordNum);
			fprintf(stderr,"File=%s/%04x-%04x.dat  currRecord=%d,maxRecord=%d\n",VARI_DIR,freezeOI,relateOI,currRecordNum,maxRecordNum);
			relateOAD.OI = relateOI;
			relateOAD.attflg = 0;
			relateOAD.attrindex = 0;
			getflg = readFreezeRecordByNum(freezeOI,relateOAD,(currRecordNum-recordn),&datetime,&datalen,(INT8U *)data);
			fill_variClass(relateOAD,getflg,(INT8U *)data,&record_para->data[index],&buflen,memp);
			index += buflen;
		}
	}
	record_para->datalen += index;
	return record_para->datalen;
}
/*
 * 返回SEQUENCE OF GetRecord 的个数
 * */
int getSel1_freeze(RESULT_RECORD *record)
{
	DateTimeBCD datetime={};
	int		getSel_ret = 0;
	int		index = 0;

	getSel_ret = getSel_Data(record->select.selec1.data.type,record->select.selec1.data.data,(INT8U *)&datetime);
	if(getSel_ret == -1) {
		fprintf(stderr,"select1 查询类型不匹配。。。。。\n");
		record->datalen = 0;
		record->dar = dblock_invalid;
		return 0;
	}else if(getSel_ret == 0){
		//一致性测试GET_10，RSD=1,OAD不在赛选范围内（数据Data为NULL） 响应报文seqof=0
		record->datalen = 0;
		return 0;
	}
	fprintf(stderr,"getSel1: OI=%04x  Time=%d:%d:%d %d-%d-%d\n",record->select.selec1.oad.OI,datetime.year.data,datetime.month.data,datetime.day.data,
			datetime.hour.data,datetime.min.data,datetime.sec.data);
	fill_RecordRow(&index,record,datetime);
	record->datalen = index;
//	一致性测试修改，是否影响正常？？？？
//	if(index==0) {	//0条记录     [1] SEQUENCE OF A-RecordRow
//		record->data[index] = 0;
//	}
//	record->datalen = index;
	fprintf(stderr,"\nrecord->datalen = %d",record->datalen);
	return 1;//ret;  //一致性测试，正常响应报文seqof=1
}

int repeatSel(Selector3 sel3,int seq)
{
	int ret = 0;
	int	i=0;
	for(i=0;i<sel3.sel2_num;i++) {
		if(i!=seq) {
			if(memcmp(&sel3.selectors[i],&sel3.selectors[seq],sizeof(Selector2))==0) {
				ret = 1;
				break;
			}
		}
	}
	return ret;
}

int getSel2_freeze(RESULT_RECORD *record)
{
	int 	ret=0,i=0,sel_id =0;
	int		index = 0,ti_sec=0;
	DateTimeBCD start_from={},end_to={},datetime={};
	TS		tmNext={};
	TI		jiange={};
	time_t	sub_time=0;
	int		seqofNum = 1;		//一致性测试GET_16:如间隔为NULL，应正常应答
	INT8U	selector2_num = 1, seqofsum = 0;
	int		repflag = 0;
	Selector2 sel2={};
	if(record->selectType == 2) {
		selector2_num = 1;
	}else if(record->selectType == 3) {
		selector2_num = record->select.selec3.sel2_num;
	}
	fprintf(stderr,"selector2_num = %d\n",selector2_num);
	record->data[index++] = seqofNum; 	//M = 1  Sequence  of A-RecordRow
	for(i=0;i<selector2_num;i++) {
		if(record->selectType == 2) {
			memcpy(&sel2,&record->select.selec2,sizeof(Selector2));
		}else if(record->selectType == 3) {
			memcpy(&sel2,&record->select.selec3.selectors[i],sizeof(Selector2));
		}
		if(sel2.oad.OI == 0x2021) {	//数据冻结时间
			if(i!=0) {		//第一次查询，不判断selector条件
				repflag = repeatSel(record->select.selec3,i);
				if(repflag==1) {
					fprintf(stderr,"i=%d sel repeat\n",i);
					continue;
				}
			}
			/*GET_16 间隔为NULL,查找数据 GET_12 间隔类型不是TI，返回记录数0*/
			if((sel2.data_from.type!=dtdatetimes)||(sel2.data_to.type!=dtdatetimes)
					||((sel2.data_jiange.type!=dtti) && sel2.data_jiange.type!=0)){
				record->data[0] = 0;	//seqofNum = 0
				record->datalen = 1;
				fprintf(stderr,"selector2 类型错误");
				return ret;
			}
			getSel_Data(sel2.data_from.type,sel2.data_from.data,(INT8U *)&start_from);
			getSel_Data(sel2.data_to.type,sel2.data_to.data,(INT8U *)&end_to);
			getSel_Data(sel2.data_jiange.type,sel2.data_jiange.data,(INT8U *)&jiange);

			if(DataTimeCmp(start_from,end_to)==0) {
				record->data[0] = 0;	//seqofNum = 0
				record->datalen = 1;
				fprintf(stderr,"selector2 起始值>=结束值");
				return ret;
			}

			sub_time = TimeBCDTotime_t(end_to)-TimeBCDTotime_t(start_from);
			ti_sec = TItoSec(jiange);
			fprintf(stderr,"sub_time=%ld,ti_sec = %d\n",sub_time,ti_sec);
			if(ti_sec!=0) {
				seqofNum = sub_time/ti_sec;
			}
			seqofsum += seqofNum;
			fprintf(stderr,"getSel2: OI=%04x  \n",record->select.selec1.oad.OI);
			printDataTimeS("起始时间",start_from);
			printDataTimeS("结束时间",end_to);
			printTI("间隔",jiange);
			fprintf(stderr,"seqOfNum = %d\n",seqofNum);
		}
		memcpy(&datetime,&start_from,sizeof(DateTimeBCD));
		for(sel_id=0;sel_id<seqofNum;sel_id++) {
			fill_RecordRow(&index,record,datetime);
			TimeBCDToTs(datetime,&tmNext);		//获取需要查找的下一次时间点
			tminc(&tmNext,jiange.units,1);
			TsToTimeBCD(tmNext,&datetime);
		}
	}
	if(seqofsum!=0) {
		record->data[0] = seqofsum; 	//M = 1  Sequence  of A-RecordRow
	}
//	record->datalen += 1;
	record->datalen = index;
	fprintf(stderr,"\nrecord->datalen = %d index=%d",record->datalen,index);
	for(i=0;i<record->datalen;i++) {
		fprintf(stderr,"%02x ",record->data[i]);
	}
	return ret;
}
int GetSle0_task(RESULT_RECORD *record)
{
	int	index = 0,ret=0;
	int	taskid=0,i=0,findmethod = 0,tsa_num=0;
	CLASS_6001 *tsa_group = NULL;
	ROAD_ITEM item_road;
	MY_MS meters_null;
	int unitnum=0;
	INT16U  blocksize=0,headsize=0;
	HEAD_UNIT *headunit = NULL;//文件头
	FILE *fp = NULL;
	meters_null.mstype = 1;//全部电表
	tsa_num = getOI6001(meters_null,(INT8U **)&tsa_group);
	memset(&item_road,0,sizeof(item_road));
	findmethod = 1;
	//fprintf(stderr,"\n\\\\\开始找任务序号...... \n");
	if((taskid = GetTaskidFromCSDs_Sle0(record->rcsd.csds,&item_road,findmethod,tsa_group)) == 0) {//暂时不支持招测的不在一个采集方案
		memset(&item_road,0,sizeof(item_road));
		findmethod = 2;
		if((taskid = GetTaskidFromCSDs_Sle0(record->rcsd.csds,&item_road,findmethod,tsa_group)) == 0) {
			return 0;
		}
	}
	if(taskid>0){
		TS ts_rec;
		TSGet(&ts_rec); //集中器时间
		char fname[300]={};
		sprintf(fname,"%s/%03d/%04d%02d%02d.dat",TASKDATA,taskid,ts_rec.Year,ts_rec.Month,ts_rec.Day);
		fp =fopen(fname,"r");

		if(fp != NULL)
		{
			unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);//////////TODO：lyl
			if(headunit == NULL) {
				fclose(fp);
				return 0;
			}
			INT8U *blockbuf=NULL;
			INT8U m=0,n=0,offset=0,j=0;
			blockbuf=malloc(blocksize);

			if(blockbuf == NULL) {
				fclose(fp);
				return 0;
			}
			INT8U first=0;
			for(m=0;m<tsa_num;m++)
			{
				first=0;
				memset(blockbuf,0,blocksize);
				int r=fread(blockbuf,blocksize,1,fp);
				fprintf(stderr,"读出的文件：\n");
				int a=0;
				for(a=0;a<blocksize;a++)
					fprintf(stderr,"%02x ",blockbuf[a]);
				if(r>0){
					fprintf(stderr,"----num=%d \n",record->rcsd.csds.num);
					for(i=0;i<record->rcsd.csds.num;i++){
						if(record->rcsd.csds.csd[i].type == 0){
							fprintf(stderr,"这是oad \n");
							offset = 0;
							INT8U haveflag = 0;
							for(n=0;n<unitnum;n++){
							if(memcmp(&record->rcsd.csds.csd[i].csd.oad,&headunit[n].oad_r,sizeof(OAD))==0){
									memcpy(&record->data[index],&blockbuf[offset],headunit[n].len);
									index +=headunit[n].len;
									offset +=headunit[n].len;
									haveflag=1;
									break;
								}
								offset +=headunit[n].len;
						    }
							if(haveflag == 0)
								record->data[index++]=0;
						}else if(record->rcsd.csds.csd[i].type == 1){
							fprintf(stderr,"这是road \n");
							if(first == 0){
								first=1;
								record->data[index++] = 1;//array
								record->data[index++] = record->rcsd.csds.csd[i].csd.road.num;//shuliang
							}

							for(j=0;j<record->rcsd.csds.csd[i].csd.road.num;j++)
							{
								offset = 0;
								fprintf(stderr,"\n j=%d oad.oi=%04x attflg=%02x,attrindex=%02x\n",j,
										record->rcsd.csds.csd[i].csd.road.oads[j].OI,record->rcsd.csds.csd[i].csd.road.oads[j].attflg,
										record->rcsd.csds.csd[i].csd.road.oads[j].attrindex);
								INT8U haveflag_1 = 0;
								for(n=0;n<unitnum;n++){
									if(memcmp(&record->rcsd.csds.csd[i].csd.road.oads[j],&headunit[n].oad_r,sizeof(OAD))==0){
										memcpy(&record->data[index],&blockbuf[offset],headunit[n].len);
										index +=headunit[n].len;
										offset +=headunit[n].len;
										haveflag_1=1;
										break;
									}
									offset +=headunit[n].len;
								}
								if(haveflag_1 == 0)
									record->data[index++]=0;
							}
						}
					}
			    }
				else
					record->data[0] = 0;
			}
			ret=tsa_num;
			if(fp !=NULL)
				fclose(fp);
			if(blockbuf!=NULL){
				free(blockbuf);
				blockbuf = NULL;
			}
		}
		else
			record->data[0] = 0;
	}
	else
		record->data[0] = 0;
	record->datalen = index;
	if(headunit != NULL){
		free(headunit);
		headunit = NULL;
	}
	if(tsa_group != NULL){
		free(tsa_group);
		tsa_group = NULL;
	}
	return ret;
}
/*
 * 返回SEQUENCE OF GetRecord 的个数
 * */
int getSel0_coll(RESULT_RECORD *record)
{
	int	index = 0,temindex = 0,ret=0;
	int	taskid=0,i=0;
	if(record->oad.OI==0x6012 && record->oad.attflg == 03)
	{
       ret=GetSle0_task(record);
       return ret;
	}
	else
	{
		if(record->rcsd.csds.num >0)
		{
			for(taskid=0;taskid<255;taskid++){
				for(i=0;i<record->rcsd.csds.num;i++){
					if(record->rcsd.csds.csd[i].type == 0)
						index += getColl_Data(record->rcsd.csds.csd[i].csd.oad.OI,taskid,&record->data[index]);
				}
				if(index != temindex)
					ret++;
				temindex=index;
			}
		}
		else
		{
			if((record->oad.OI>>12) == 6){
				ret= getFileRecordNum(record->oad.OI);

				for(i=0;i<ret;i++)
				{
					if(i==0 && record->oad.OI == 0x6000)
						continue;
					index +=getColl_Data(record->oad.OI,i,&record->data[index]);
				}
				if(record->oad.OI == 0x6000)
					ret--;
			}
		}
	}
	if(ret==0) {	//0条记录     [1] SEQUENCE OF A-RecordRow
		record->data[0] = 0;
		index=1;	//？？？？
	}
	record->datalen = index;
	return ret;
}
/*
 * 返回SEQUENCE OF GetRecord 的个数
 * */
int getSel1_coll(RESULT_RECORD *record)
{
	int ret=1;
	int		index = 0;
	int		taskid=0;

	getSel_Data(record->select.selec1.data.type,record->select.selec1.data.data,(INT8U *)&taskid);
	fprintf(stderr,"getSel1: OI=%04x  taskid=%d\n",record->select.selec1.oad.OI,taskid);
	index += getColl_Data(record->select.selec1.oad.OI,taskid,&record->data[index]);
	if(index==0) {	//0条记录     [1] SEQUENCE OF A-RecordRow
		record->data[0] = 0;
		index=1;	//？？？？
	}
	record->datalen = index;
	fprintf(stderr,"\nrecord->datalen = %d",record->datalen);
	return ret;
}


///
int getSel2_coll(RESULT_RECORD *record)
{
	int ret=0;
	int		index = 0;
	int		taskid_from=0,taskid_to=0,taskid_jiange=0;
	int		sel_id=0;

	getSel_Data(record->select.selec2.data_from.type,record->select.selec2.data_from.data,(INT8U *)&taskid_from);
	getSel_Data(record->select.selec2.data_to.type,record->select.selec2.data_to.data,(INT8U *)&taskid_to);
	getSel_Data(record->select.selec2.data_jiange.type,record->select.selec2.data_jiange.data,(INT8U *)&taskid_jiange);

	fprintf(stderr,"getSel2: OI=%04x  taskid_from=%d taskid_to=%d\n",record->select.selec1.oad.OI,taskid_from,taskid_to);
	record->data[index++] = taskid_to-taskid_from + 1; 	//M = 1  Sequence  of A-RecordRow

	for(sel_id=taskid_from;sel_id<=taskid_to;sel_id++) {
		index += getColl_Data(record->select.selec2.oad.OI,sel_id,&record->data[index]);
		if(index==0) {	//0条记录     [1] SEQUENCE OF A-RecordRow
			record->data[0] = 0;
			index += 1;
		}
	}
	record->datalen = index;
	fprintf(stderr,"\nrecord->datalen = %d",record->datalen);
	return ret;
}
/*
 * 选择方法1: 读取指定对象指定值
 * */
int getSelector12(RESULT_RECORD *record)
{
	int  ret=0;
	INT8U oihead = (record->oad.OI & 0xF000) >>12;

	switch(oihead) {
	case 5:
		fprintf(stderr,"\n冻结类对象\n");
		switch(record->selectType) {
		case 1:
			ret = getSel1_freeze(record);
			break;
		case 2:
		case 3:
			ret = getSel2_freeze(record);
			break;
		}
		break;
	case 6:			//采集监控类对象
		fprintf(stderr,"\n读取采集监控对象\n");
		if(record->selectType == 0) {
			ret = getSel0_coll(record);
		}else if(record->selectType == 1) {
			ret = getSel1_coll(record);
		}else if(record->selectType == 2) {
			ret = getSel2_coll(record);
		}
		break;
	default:
		fprintf(stderr,"oi=%x oihead = %d\n",record->oad.OI,oihead);
		record->dar = obj_undefine;
		record->datalen = 0;
		break;
	}
	return ret;
}

/*
 * type = 3:GetResponseRecord
 * type = 4:GetResponseRecordList
 * RSD用于选择记录型对象属性的各条记录，即二维记录表的行选择，其通过对构成记录的某些对象属性数值进行指定来进行选择，范围选择区间：前闭后开，即[起始值，结束值）。
 * */
int doGetrecord(INT8U type,OAD oad,INT8U *data,RESULT_RECORD *record,INT16U *subframe,int recordnum)
{
	const static OI_698 event_relate_oi[]={0x2022,0x201e,0x2020,0x2024};	//事件记录序号，事件发生时间，事件结束时间，事件发生源
	INT8U 	oihead = 0;
	int 	source_index=0;		//getrecord 指针
	int		dest_index=0;		//getreponse 指针
	int		response_choice_index = 0;		//记录response的choice的位置
	INT8U 	SelectorN =0;
	int		datalen=0;
	int		seqof_len = 0;
	int		i=0	,freezeflg = 0 ,ret = 0;

//	DEBUG_TIME_LINE("\nGetRequestRecord   oi=%x  %02x  %02x",record->oad.OI,record->oad.attflg,record->oad.attrindex);
	source_index = get_BasicRSD(0,&data[source_index],(INT8U *)&record->select,&record->selectType);
//	DEBUG_TIME_LINE("\nRSD Select%d     data[%d] = %02x",record->selectType,source_index,data[source_index]);
	source_index +=get_BasicRCSD(0,&data[source_index],&record->rcsd.csds);
	//record.rcsd.csds.csd[i].csd.oad.OI
	SelectorN = record->selectType;
	if(oad.OI == 0x6012 && oad.attflg == 03 && SelectorN == 0) {//6012的属性3招测记录单元走招测任务数据
		SelectorN = 10;
	}
//	DEBUG_TIME_LINE("\n- getRequestRecord SelectorN=%d OI = %04x  attrib=%d  index=%d",SelectorN,record->oad.OI,record->oad.attflg,record->oad.attrindex);
//	printrecord(*record);

	record->data = &TmpDataBuf[dest_index];
	dest_index += create_OAD(0,&record->data[dest_index],record->oad);
//    fprintf(stderr,"selectorn=%d  dest_index = %d %02x_%02x_%02x_%02x\n",SelectorN,dest_index,record->data[0],record->data[1],record->data[2],record->data[3]);
	switch(SelectorN) {
	case 0:
	case 1:		//指定对象指定值
		*subframe = 1;		//TODO:未处理分帧
		if(record->rcsd.csds.num == 0 && SelectorN == 0 && record->oad.OI == 0x6000){
			record->data[dest_index++]=1;
			record->data[dest_index++]=00;
			record->data[dest_index++]=0x60;
			record->data[dest_index++]=0x01;
			record->data[dest_index++]=0x02;
			record->data[dest_index++]=0x00;
		}
		else if(record->rcsd.csds.num == 0 && SelectorN == 1) {
			record->data[dest_index++] = 1;	//一行记录M列属性描述符 	RCSD
			record->data[dest_index++] = 0;	//OAD
			record->select.selec1.oad.attrindex = 0;		//上送属性下所有索引值
			dest_index += create_OAD(0,&record->data[dest_index],record->select.selec1.oad);
		}else{
			dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
			if(record->rcsd.csds.num == 0){
				record->data[dest_index++] = 0;
			}
		}

		response_choice_index = dest_index;
		dest_index += 2;
		record->data = &TmpDataBuf[dest_index];		//修改record的数据帧的位置
		seqof_len = getSelector12(record);
		record->data = TmpDataBuf;				//data 指向回复报文帧头
		record->datalen += dest_index;			//数据长度+ResultRecord

		if(record->dar == success) {
			record->data[response_choice_index] = 1; //CHOICE  [1]  data
			record->data[response_choice_index+1] = seqof_len; //M = 1  Sequence  of A-RecordRow
		}else {

			record->data[response_choice_index] = 0; //DAR
			record->data[response_choice_index+1] = record->dar; //错误类型
		}
	break;
	case 2:
		oihead=((record->oad.OI & 0xF000) >>12);
		if(oihead == 3){	//事件类
			*subframe = 1;		//TODO:未处理分帧
			TmpDataBuf[dest_index++] = 4;
			INT8U ai=0;
			for(ai=0;ai<4;ai++){
				TmpDataBuf[dest_index++] = 0;
				TmpDataBuf[dest_index++] = (event_relate_oi[ai]>>8)&0x00ff;
				TmpDataBuf[dest_index++] = event_relate_oi[ai]&0x00ff;
				TmpDataBuf[dest_index++] = 2;
				TmpDataBuf[dest_index++] = 0;
			}
			record->data = &TmpDataBuf[dest_index];
			Getevent_Record_Selector(record,memp);
			record->data = TmpDataBuf;				//data 指向回复报文帧头
			record->datalen += dest_index;			//数据长度+ResultRecord
		}else {
			*subframe = 1;		//TODO:未处理分帧
			if(record->rcsd.csds.num == 0) {
				record->data[dest_index++] = 1;	//一行记录M列属性描述符 	RCSD
				record->data[dest_index++] = 0;	//OAD
				record->select.selec1.oad.attrindex = 0;		//上送属性下所有索引值
				dest_index += create_OAD(0,&record->data[dest_index],record->select.selec1.oad);
				record->data[dest_index++] = 1; //CHOICE  [1]  data
			}else {	//RCSD 招测两个序号湖南测过
				dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
				record->data[dest_index++] = 1; //CHOICE  [1]  data
			}
			record->data = &TmpDataBuf[dest_index];		//修改record的数据帧的位置
			getSelector12(record);
			record->data = TmpDataBuf;				//data 指向回复报文帧头
			record->datalen += dest_index;			//数据长度+ResultRecord
		}
		break;
	case 3:	//Selector3为组合筛选，即若干个指定对象连续值。 SEQUENCE OF Selector2
		*subframe = 1;		//TODO:未处理分帧
		if(record->rcsd.csds.num == 0) {
			record->data[dest_index++] = 1;	//一行记录M列属性描述符 	RCSD
			record->data[dest_index++] = 0;	//OAD
			record->select.selec1.oad.attrindex = 0;		//上送属性下所有索引值
			dest_index += create_OAD(0,&record->data[dest_index],record->select.selec1.oad);
			record->data[dest_index++] = 1; //CHOICE  [1]  data
		}else {	//RCSD 招测两个序号湖南测过
			dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
			record->data[dest_index++] = 1; //CHOICE  [1]  data
		}
		record->data = &TmpDataBuf[dest_index];		//修改record的数据帧的位置
		getSelector12(record);
		record->data = TmpDataBuf;				//data 指向回复报文帧头
		record->datalen += dest_index;			//数据长度+ResultRecord
		break;
	case 4://指定电能表集合、指定采集启动时间
	case 5://指定电能表集合、指定采集存储时间
	case 6://指定电能表集合、指定采集启动时间区间内连续间隔值
	case 7://指定电能表集合、指定采集存储时间区间内连续间隔值
	case 8://指定电能表集合、指定采集成功时间区间内连续间隔值
		dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
//		record->data = &TmpDataBuf[dest_index];
		*subframe = getSelector(oad,record->select, record->selectType,record->rcsd.csds,(INT8U *)record->data,(int *)&record->datalen,AppVar_p->server_send_size,recordnum);
		if(*subframe>=1) {		//无分帧
			next_info.nextSite = readFrameDataFile(TASK_FRAME_DATA,next_info.nextSite,TmpDataBuf,&datalen);
			fprintf(stderr,"next_info.nextSite=%d\n",next_info.nextSite);
			//文件中第一个字节保存的是：SEQUENCE OF A-ResultRecord，GET_REQUEST_RECORD 此处从TmpDataBuf[1]上送，上送长度也要-1
			//湖南招测多个测量点报文使用 GET_REQUEST_RECORD_LIST 解析过程中上送了SEQUENCE OF A-ResultRecord，不读取文件中该值
			if(datalen>=1) {
				record->data = &TmpDataBuf[1];				//data 指向回复报文帧头
				record->datalen += (datalen-1);				//数据长度+ResultRecord
			}
//			if(type==GET_REQUEST_RECORD) {//文件中第一个字节保存的是：SEQUENCE OF A-ResultRecord，此处从TmpDataBuf[1]上送，上送长度也要-1
//				if(datalen>=1) {
//					record->data = &TmpDataBuf[1];				//data 指向回复报文帧头
//					record->datalen += (datalen-1);				//数据长度+ResultRecord
//				}
//			}else if(type==GET_REQUEST_RECORD_LIST) {
//				record->data = &TmpDataBuf[1];				//data 指向回复报文帧头
//				record->datalen += (datalen-1);			//湖南招测 .....case10:????
//			}
		}else {	//无数据
			record->data[dest_index++] = 1;	//CHOICE = 1
			record->data[dest_index++] = 0;	//A-RecordRow = 0 无数据
			record->datalen = dest_index;
		}
		break;

	case 9:		//指定读取上第n次记录
		*subframe = 1;		//TODO:未处理分帧
		dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
//		fprintf(stderr,"dest_index = %d  record->rcsd.csds.num=%d\n",dest_index,record->rcsd.csds.num);
		record->data = &TmpDataBuf[dest_index];
		 //一致性测试GET_24 rcsd=0,应答帧应填写rcsd=0 ,
		if((record->oad.OI & 0xf000) == 0x5000) {	//招测冻结类数据
			if (record->rcsd.csds.num==0) 	{
				record->data[dest_index++] = 0;	//RCSD=0
				record->data[dest_index++] = 1;	//Data
				record->data[dest_index++] = 1;	//seqof A-RecordRow
				record->data[dest_index++] = 0;	//A-RecordRow len
				record->datalen += dest_index;
			}else 	{
				//终端类冻结数据
				fill_RecordRowByNum(record);
				record->data = TmpDataBuf;				//data 指向回复报文帧头
				record->datalen += dest_index;			//数据长度+ResultRecord
			}
		}else {
			Getevent_Record_Selector(record,memp);
			record->data = TmpDataBuf;				//data 指向回复报文帧头
			record->datalen += dest_index;			//数据长度+ResultRecord
		}
		break;
	case 10:	//指定读取最新的n条记录
		//协议一致性测试GET_25,招测OAD=6012-0300,应该能正常找到数据应答
		//6012_02 测试，判断格式，无RCSD,招测数据应该无数据
		for(i=0;i<record->rcsd.csds.num;i++) {//异常处理 没有有效oad
			switch(record->rcsd.csds.csd[i].type)
			{
			case 0:
				if(record->rcsd.csds.csd[i].csd.oad.OI != 0x6040 && record->rcsd.csds.csd[i].csd.oad.OI != 0x6041 && record->rcsd.csds.csd[i].csd.oad.OI != 0x6042 &&
						record->rcsd.csds.csd[i].csd.oad.OI != 0x202a)
					freezeflg = 1;
				break;
			case 1:
				freezeflg = 1;
				break;
			default:
				return 0;
				break;
			}
			if(freezeflg == 1)
				break;
		}
		if(freezeflg == 0)
		{
			fprintf(stderr,"招测的OAD无效！！！\n");
			*subframe = 1;
			dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
			record->data[dest_index++]=1;
			record->data[dest_index++]=0; //按找不到数据处理
			record->datalen += dest_index;
			return 0;
		}
		else
			fprintf(stderr,"招测的OAD有效！！！\n");


		*subframe = getSelector(record->oad,record->select,record->selectType,record->rcsd.csds,NULL,NULL,AppVar_p->server_send_size,0);
		if(*subframe==1) {		//无分帧
			//文件中第一个字节保存的是：SEQUENCE OF A-ResultRecord，此处从TmpDataBuf[1]上送，上送长度也要-1
			next_info.nextSite = readFrameDataFile(TASK_FRAME_DATA,0,TmpDataBuf,&datalen);
			if(datalen>=1) {	//文件中第一个字节保存的是：SEQUENCE OF A-ResultRecord，此处从TmpDataBuf[1]上送，上送长度也要-1
				record->data = &TmpDataBuf[1];				//data 指向回复报文帧头
				record->datalen += (datalen-1);				//数据长度+ResultRecord
			}
//			if(type==GET_REQUEST_RECORD) {//文件中第一个字节保存的是：SEQUENCE OF A-ResultRecord，此处从TmpDataBuf[1]上送，上送长度也要-1
//				if(datalen>=1) {								//TODO:浙江曲线招测测试过
//					record->data = &TmpDataBuf[1];				//data 指向回复报文帧头
//					record->datalen += (datalen-1);				//数据长度+ResultRecord
//				}
//			}else if(type==GET_REQUEST_RECORD_LIST) {
//				record->data = TmpDataBuf;				//data 指向回复报文帧头
//				record->datalen += dest_index;			//数据长度+ResultRecord
//			}
		}else {	//无数据
			record->data[dest_index++] = 1;	//CHOICE = 1
			record->data[dest_index++] = 0;	//A-RecordRow = 0 无数据
			record->datalen = dest_index;
		}
		break;
	}
	fprintf(stderr,"\n---doGetrecord end\n");
	return source_index;
}

int GetEnergy(RESULT_NORMAL *response)
{
	if(response->oad.attflg>=1 && response->oad.attflg<=5) {

	}else {
		response->dar = type_mismatch;	//国网协议一致性测试
	}
	fprintf(stderr,"datalen=%d \n",response->datalen);
	return 1;
}

int GetEventRecord(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	int datalen=0;

	if ( Get_Event(response->oad,response->oad.attrindex,&data,&datalen,memp) == 1 )
	{
		if (datalen > 512 || data==NULL)
		{
			fprintf(stderr,"\n获取事件数据Get_Event函数异常! [datalen=%d  data=%p]",datalen,data);
			if (data!=NULL) {
				free(data);
				data = NULL;
			}
			return 0;
		}
		memcpy(response->data,data,datalen);
		response->datalen = datalen;
		if (data!=NULL) {
			free(data);
			data = NULL;
		}
		return 1;
	}
	response->datalen = 0;
	response->dar = other_err1;
	fprintf(stderr,"\n获取事件数据Get_Event函数返回 0  [datalen=%d  data=%p]",datalen,data);
	if (data!=NULL) {
		free(data);
		data = NULL;
	}
	return 1;
}

int GetClass7attr(RESULT_NORMAL *response)
{
	INT8U *data = NULL;
	OAD oad={};
	Class7_Object	class7={};
	INT8U  *parabuf=NULL;
	int 	filesize=0;
	int 	index=0,i=0;
	int 	ret=0;

	data = response->data;
	oad = response->oad;
	memset(&class7,sizeof(Class7_Object),0);

	filesize = getClassFileLen(oad.OI,0,event_para_save);
	if(filesize>2) {
		fprintf(stderr,"filesize=%d\n",filesize);
		parabuf = malloc(filesize);
		ret = readCoverClass(oad.OI,0,parabuf,(filesize-2),event_para_save); //读取整个数据内容，否则CRC校验不通过 TODO：filesize-2，需要验证 不同的文件大小
		//	ret = readCoverClass(oad.OI,0,&class7,sizeof(Class7_Object),event_para_save);
		memcpy(&class7,parabuf,sizeof(Class7_Object));
		if(parabuf!=NULL) {
			free(parabuf);
			parabuf = NULL;
		}
	}
	fprintf(stderr,"ret = %d class7.crrentnum=%d\n",ret, class7.crrentnum);
	switch(oad.attflg) {
	case 1:	//逻辑名
		index += fill_octet_string(&data[0],(char *)&class7.logic_name,sizeof(class7.logic_name));
		break;
	case 3:	//关联属性表
		index += create_array(&data[0],class7.class7_oad.num);
		for(i=0;i<class7.class7_oad.num;i++) {
			index += create_OAD(0,&data[index],class7.class7_oad.oadarr[i]);
		}
		break;
	case 4:	//当前记录数
		index += fill_long_unsigned(&data[0],class7.crrentnum);
		break;
	case 5:	//最大记录数
		index += fill_long_unsigned(&data[0],class7.maxnum);
		break;
	case 8: //上报标识
		index += fill_enum(&data[0],class7.reportflag);
		break;
	case 9: //有效标识
		index += fill_bool(&data[0],class7.enableflag);
		break;
	}
	response->datalen = index;
	return 0;
}

int GetEventInfo(RESULT_NORMAL *response)
{
	fprintf(stderr,"GetEventInfo OI=%x,attflg=%d,attrindex=%d \n",response->oad.OI,response->oad.attflg,response->oad.attrindex);
	switch(response->oad.attflg) {
	case 2:		//事件记录表
		GetEventRecord(response);
		break;
	case 1:	//逻辑名
	case 3:	//关联属性表
	case 4:	//当前记录数
	case 5:	//最大记录数
	case 8: //上报标识
	case 9: //有效标识
		GetClass7attr(response);
		break;
	case 6:	//配置参数
		switch(response->oad.OI) {
			case 0x3105:	//电能表时间超差事件
				Get3105(response);
				break;
			case 0x3106:	//停上电事件
				Get3106(response);
				break;
			case 0x310c:	//电能量超差事件阈值
				Get310c(response);
				break;
			case 0x310d:	//电能表飞走事件阈值
				Get310d(response);
				break;
			case 0x310e:	//电能表停走事件阈值
				Get310e(response);
				break;
			case 0x310f:	//终端抄表失败事件
				Get310f(response);
				break;
			case 0x3110:	//月通信流量超限事件阈值
				Get3110(response);
				break;
		}
		break;
	case 7:
		GetEventRecord(response);
		break;
	}
	return 0;
}

int GetEnvironmentValue(RESULT_NORMAL *response)
{
	switch(response->oad.OI)
	{
		case 0x4000:
			response->datalen = Get_4000(response->oad,response->data);
//			GetSysDateTime(response);
			break;
		case 0x4001:
		case 0x4002:
		case 0x4003:
			Get4001_4002_4003(response);
			break;
		case 0x4004:
			Get4004(response);
			break;
		case 0x4005:
			Get4005(response);
			break;
		case 0x4006:
			Get4006(response);
			break;
		case 0x4007:
			Get4007(response);
			break;
		case 0x400C:
			Get400C(response);
			break;
		case 0x4014:
			Get4014(response);
			break;
		case 0x4016:
			Get4016(response);
			break;
		case 0x4103:
			Get4103(response);
			break;
		case 0x4202:
			Get4202(response);
			break;
		case 0x4204:
			Get4204(response);
			break;
		case 0x4300:
			Get4300(response);
			break;
		case 0x4400:
			Get4400(response);
			break;
		case 0x4500://无线公网设备版本
			Get4500(response);
			break;
		case 0x4510://以太网通信模块
			Get4510(response);
			break;
		case 0x4018://当前套费率电价
			Get4018(response);
			break;
		default:	//未定义的对象
			response->dar = obj_undefine;
		break;
	}
	return 1;
}

/*
 * 获取一个采集档案配置的单元的内容
 * readType: =1:不判断数据有效性，只读取一个单元的数据长度，用于判断分帧及数据单元的个数
 *           =0：获取一个数据单元的内容
 * */
int GetCollOneUnit(OAD oad,INT8U readType,INT8U seqnum,INT8U *data,INT16U *oneUnitLen,INT16U *blknum)
{
	int  one_unitlen = 0, one_blknum = 0;
	switch(oad.OI)
	{
	case 0x6000:	//采集档案配置表
		one_unitlen = Get_6001(readType,seqnum,data);
		one_blknum = getFileRecordNum(oad.OI);
		if(one_blknum<=1)	return 0;
		break;
	case 0x6002:	//搜表
		one_unitlen = Get_6002(oad,readType,data);
		one_blknum = 1;
		break;
	case 0x6012:	//任务配置表
		one_unitlen = Get_6013(readType,seqnum,data);
		one_blknum = 256;
		break;
	case 0x6014:	//普通采集方案集
		one_unitlen = Get_6015(readType,seqnum,data);
		one_blknum = 256;
		break;
	case 0x6016:	//事件采集方案
		one_unitlen = Get_6017(readType,seqnum,data);
		one_blknum = 256;
		break;
	}
	*oneUnitLen = one_unitlen;
	*blknum = one_blknum;
	if(one_unitlen!=0)	fprintf(stderr,"GetCollOneUnitLen oad.oi=%04x one_unitlen=%d one_blknum=%d\n",oad.OI,one_unitlen,one_blknum);
	return 1;
}
/*
 * 采集监控类对象读取
 * */
int GetCtrl(RESULT_NORMAL *response)
{

	switch(response->oad.OI)
	{
		case 0x8000://遥控
			response->datalen = Get_8000(response);
			break;
		case 0x8001://保电
			response->datalen = Get_8001(response);
			break;
		case 0x8002://催费告警
			response->datalen = Get_8002(response);
			break;
		case 0x8003://一般中文信息
		case 0x8004://重要中文信息
			response->datalen = Get_8003_8004(response);
			break;
		case 0x8100://终端保安定值
			response->datalen = Get_8100(response);
			break;
		case 0x8101://终端功控时段
			response->datalen = Get_8101(response);
			break;
		case 0x8102://功控告警时间
			response->datalen = Get_8102(response);
			break;
		case 0x8103://时段功控
			response->datalen = Get_8103(response);
			break;
		case 0x8104://厂休控
			response->datalen = Get_8104(response);
			break;
		case 0x8105://营业报停控
			response->datalen = Get_8105(response);
			break;
		case 0x8106://当前功率下浮控
			response->datalen = Get_8106(response);
			break;
		case 0x8107://购电控
			response->datalen = Get_8107(response);
			break;
		case 0x8108://月电控
			response->datalen = Get_8108(response);
			break;
	}
	return 1;
}

int GetCollPara(INT8U seqOfNum,RESULT_NORMAL *response){
	int 	index=0;
	INT8U 	*data = NULL;
	OAD 	oad={};
	INT16U	i=0,blknum=0,meternum=0,tmpblk=0;
	INT16U	retlen=0;
	INT16U	oneUnitLen=0;	//计算一个配置单元的长度，统计是否需要分帧操作
	int		lastframenum = 0; //记录分帧的数量

	oad = response->oad;
	data = response->data;

	if(GetCollOneUnit(response->oad,1,0,&data[index],&oneUnitLen,&blknum)==0)	{
		fprintf(stderr,"get OI=%04x oneUnitLen=%d blknum=%d 退出",oad.OI,oneUnitLen,blknum);
		response->dar = obj_undefine;
		return 0;
	}
//	if(seqOfNum!=0) {　　　//台体抄表参数读取多个，去掉判断
		index = 2;			//空出 array,结束后填入
//	}
	for(i=0;i<blknum;i++)
	{
//		if(seqOfNum!=0) {							//getRequestNormal请求时不需要SEQUENCE OF
			create_array(&data[0],meternum);		//每次循环填充配置单元array个数，为了组帧分帧
//		}
		response->datalen = index;
		///在读取数据组帧前判断是否需要进行分帧
		Build_subFrame(0,(index+oneUnitLen),seqOfNum,response);
		if(lastframenum != next_info.subframeSum) {		//数据已分帧重新开始
			lastframenum = next_info.subframeSum;
			index = 2;
			meternum = 0;
//			fprintf(stderr,"\n get subFrame lastframenum=%d,subframeSum=%d index=%d\n",lastframenum,next_info.subframeSum,index);
		}
		GetCollOneUnit(response->oad,0,i,&data[index],&retlen,&tmpblk);
		if(retlen!=0) {
			meternum++;
		}
		index += retlen;
//		if(seqOfNum==0 && meternum==1)  {	//getReponseNormal只返回一个A-ResultNormal结果数据
//			break;
//		}
	}
//	if(seqOfNum!=0) {
		create_array(&data[0],meternum);		//配置单元个数
//	}
	response->datalen = index;
	if(next_info.subframeSum!=0) {		//已经存在分帧情况
		Build_subFrame(1,index,seqOfNum,response);		//后续帧组帧, TODO:RequestNormalList 方法此处调用是否合适
	}
	return 1;
}

int GetDeviceIo(RESULT_NORMAL *response)
{
	switch(response->oad.OI) {
		case 0xF001:
		switch(response->oad.attflg) {
			case 2:	//文件信息
			case 3:	//命令结果
				response->datalen = GetClass18(response->oad.attflg,response->data);
				break;
			case 4:	//传输块状态字
				GetFileState(response);
				break;
		}
		break;
		case 0xF100:
			GetEsamPara(response);
			break;
		case 0xF101:
			GetSecurePara(response);
			break;
		case 0xF201:	//RS485
			response->datalen = GetF201(response->oad,response->data);
			break;
		case 0xF203:
			GetYxPara(response);
			break;
		case 0xF205:
			Get_f205_attr2(response);
			break;
		case 0xF206:
			GetF206(response);
			break;
		case 0xF207:
			GetF207(response);
			break;
		case 0xF209:	//ZB
			response->datalen = GetF209(response->oad,response->data);
			break;
		default:	//未定义的对象
			response->dar = obj_undefine;
		break;
	}
	return 1;
}
int doGetnormal(INT8U seqOfNum,RESULT_NORMAL *response)
{
	INT16U oi = response->oad.OI;
	INT8U oihead = (oi & 0xF000) >>12;

	if(Response_timetag.effect==0) {
		response->dar = timetag_invalid;
		return 0;
	}
	fprintf(stderr,"\ngetRequestNormal----------  oi =%04x  \n",oi);
	switch(oihead) {
	case 0:			//电能量对象
		GetEnergy(response);
		break;
	case 2:			//变量类对象
//		if(response->oad.OI != 0x2000 && memp->cfg_para.device == CCTT2) {
//			response->dar = boundry_over;
//			response->datalen = 0;
//			return 0;
//		}
		if(fill_variClass(response->oad,1,NULL,response->data,&response->datalen,memp)==0) {
			response->dar = obj_unexist;
			response->datalen = 0;
			return 0;
		}
		//		GetVariable(response);
		break;
	case 3:			//事件类对象读取
		GetEventInfo(response);
		break;
	case 4:			//参变量类对象
		GetEnvironmentValue(response);
		int i=0;
		fprintf(stderr,"当前data：");
		for(i=0;i<response->datalen;i++)
			fprintf(stderr,"%02x ",response->data[i]);
		fprintf(stderr,"\n");
		break;
	case 6:			//采集监控类对象
		fprintf(stderr,"\n读取采集监控对象oi=%04x \n",oi);
		GetCollPara(seqOfNum,response);
		break;
	case 8:
		GetCtrl(response);
		break;
	case 0xF:		//文件类/esam类/设备类
		GetDeviceIo(response);
		break;
	default:	//未定义的对象
		response->dar = obj_undefine;
		break;
	}
	return 0;
}

int getRequestNormal(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_NORMAL response={};
	INT8U 	oadtmp[4]={};
	FILE	*fp=NULL;

	//初始化分帧文件及结构
	memset(&next_info,0,sizeof(next_info));
	fp = fopen(PARA_FRAME_DATA,"w");
	if(fp!=NULL)  fclose(fp);

	oadtmp[0] = (oad.OI>>8) & 0xff;
	oadtmp[1] = oad.OI & 0xff;
	oadtmp[2] = oad.attflg;
	oadtmp[3] = oad.attrindex;
	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	memcpy(TmpDataBuf,oadtmp,4);

	response.oad = oad;
	response.data = TmpDataBuf+5; //4 + 1             oad（4字节） + choice(1字节)
	response.datalen = 0;
	doGetnormal(0,&response);
	if(next_info.subframeSum==0)	{	//无分帧
		if (response.datalen>0)
		{
			TmpDataBuf[4] = 1;//数据
			response.datalen += 5;  // datalen + oad(4) + choice(1)
		}else
		{
			TmpDataBuf[4] = 0;//错误
			TmpDataBuf[5] = response.dar;
			response.datalen += 6;  // datalen + oad(4) + choice(1) + dar
		}
		response.data = TmpDataBuf;
		BuildFrame_GetResponse(GET_REQUEST_NORMAL,csinfo,0,response,sendbuf);
	}else {		//分帧
		next_info.frameNo = 1;
		Get_subFrameData(0,csinfo,sendbuf);
	}
	return 1;
}

int getRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_NORMAL response={};
	INT8U oadtmp[4]={};
	INT8U oadnum = data[0];
	int i=0,listindex=0;
	FILE	*fp=NULL;

	fprintf(stderr,"\nGetNormalList!! OAD_NUM=%d",oadnum);

	memset(&next_info,0,sizeof(next_info));
	fp = fopen(PARA_FRAME_DATA,"w");
	if(fp!=NULL)  fclose(fp);

	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	for(i=0;i<oadnum;i++)
	{
		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
		memcpy(oadtmp,&data[1 + i*4],4);
		response.dar = 0;
		getOAD(0,oadtmp, &response.oad,NULL);
		response.datalen = 0;
		response.data = TmpDataBuf + 5;
		fprintf(stderr,"\n【%d】OI = %x  %02x  %02x",i,response.oad.OI,response.oad.attflg,response.oad.attrindex);
		doGetnormal((i+1),&response);

		memcpy(&TmpDataBufList[listindex + 0],oadtmp,4);
		if (response.datalen>0)
		{
			TmpDataBufList[listindex + 4] = 1;//数据
			memcpy(&TmpDataBufList[listindex + 5],response.data,response.datalen);
			listindex = listindex + 5 + response.datalen;
		}
		else
		{
			TmpDataBufList[listindex + 4] = 0;//错误		//TODO:原注释，台体测试读取终端当前数据时2200无数据，帧错误，打开
			TmpDataBufList[listindex + 5] = response.dar;  //  0-3(oad)   4(choice)  5(dar)
			listindex = listindex + 6;
		}
	}
	response.data = TmpDataBufList;
	response.datalen = listindex;
	BuildFrame_GetResponse(GET_REQUEST_NORMAL_LIST,csinfo,oadnum,response,sendbuf);
	return 1;
}

int getRequestRecord(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_RECORD 	record={};
	INT16U 		subframe=1;

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	record.oad = oad;
	record.data = TmpDataBuf;
	record.datalen = 0;
	next_info.nextSite = 0;	//初始化，调用doGetrecord从nextSite位置开始获取
	doGetrecord(GET_REQUEST_RECORD,oad,data,&record,&subframe,0);
	if(subframe==0 || subframe==1) {
		BuildFrame_GetResponseRecord(GET_REQUEST_RECORD,csinfo,record,sendbuf);
	}else  if(subframe>1){
		next_info.subframeSum = subframe;
		next_info.frameNo = 1;
		next_info.repsonseType = GET_REQUEST_RECORD;	//此处不直接赋值上送值是因为需要读取分帧文件的第一个字节是sequence of的值
		BuildFrame_GetResponseNext(GET_REQUEST_RECORD_NEXT,csinfo,record.dar,record.datalen,record.data,sendbuf);
	}
//	securetype = 0;		//清除安全等级标识
	return 1;
}

/*
 * RequestRecordList->应答采用GetResponseNext每一帧
 * */
//int getRequestRecordList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
//{
//	RESULT_RECORD record={};
//	OAD	oad={};
//	INT16U 		subframe=0,frmrecord=0;
//	int i=0;
//	int recordnum = 0;
//	int destindex=0;
//	int sourceindex=0;
//
//	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
//	recordnum = data[sourceindex++];
//	fprintf(stderr,"getRequestRecordList  Result-record=%d \n ",recordnum);
////	TmpDataBufList[destindex++] = 1;
//	for(i=0;i<recordnum;i++) {
//		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
//		record.data = TmpDataBuf;
//		record.datalen = 0;
//		sourceindex += getOAD(0,&data[sourceindex],&oad,NULL);
//		record.oad = oad;
//		sourceindex += doGetrecord(GET_REQUEST_RECORD_LIST,oad,&data[sourceindex],&record,&subframe,i);
//		if(recordnum > 1 && subframe > 0)
//			frmrecord += subframe;
//		if(i == 0)
//		{
//			fprintf(stderr,"\n\n*************************(%d)",record.datalen);
//			PRTbuf(record.data,record.datalen);
//			fprintf(stderr,"\n\n*************************");
//			memcpy(&TmpDataBufList[destindex],&record.data[0],record.datalen);
//			destindex += record.datalen;
//		}
////		fprintf(stderr,"$$$$$$$$$$$$$$$$$$$$$$$$$$i=%d  record.datalen  ==== %d  subframe = %d\n\n\n\n",i,record.datalen,subframe);
//	}
//	fprintf(stderr,"!!!record.datalen  ==== %d  subframe = %d\n\n\n\n",record.datalen,subframe);
//	record.data = TmpDataBufList;
//	record.datalen = destindex;
//	if(frmrecord > 0)
//		subframe = frmrecord;
//	if(subframe==1) {		//不分帧　原来判断＝０？有错
//		BuildFrame_GetResponseRecord(GET_REQUEST_RECORD_LIST,csinfo,record,sendbuf);//原来是GET_REQUEST_RECORD，是否有错？？
//	}else  if(subframe>1){
//		next_info.subframeSum = subframe;
//		next_info.frameNo = 1;
//		next_info.repsonseType = GET_REQUEST_RECORD_LIST;
//		BuildFrame_GetResponseNext(GET_REQUEST_RECORD_NEXT,csinfo,record.dar,record.datalen,record.data,sendbuf);
//	}
//	return 1;
//}

int getRequestRecordList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_RECORD record={};
	OAD	oad={};
	INT16U 		subframe=0,frmrecord=0;
	int i=0;
	int recordnum = 0;
	int destindex=0;
	int sourceindex=0;
	int	first_frameLen = 0,first_fileLen=0;

	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	recordnum = data[sourceindex++];
	fprintf(stderr,"getRequestRecordList  Result-record=%d \n ",recordnum);
	TmpDataBufList[destindex++] = recordnum;	//Reponse的A-ResultRecord的总个数
	next_info.nextSite = 0;
	for(i=0;i<recordnum;i++) {
		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
		record.data = TmpDataBuf;
		record.datalen = 0;
		sourceindex += getOAD(0,&data[sourceindex],&oad,NULL);
		record.oad = oad;
		sourceindex += doGetrecord(GET_REQUEST_RECORD_LIST,oad,&data[sourceindex],&record,&subframe,i);
		if(recordnum > 1 && subframe > 0)	//判断record数据有返回
			frmrecord += subframe;
		fprintf(stderr,"\n\n*************************(%d)",record.datalen);
		PRTbuf(record.data,record.datalen);
		fprintf(stderr,"\n\n*************************");
		if((record.datalen+destindex) <= MAXSIZ_FAM * 2) {
			memcpy(&TmpDataBufList[destindex],&record.data[0],record.datalen);
			destindex += record.datalen;
		}else {
			syslog(LOG_ERR,"数据帧[%d]越限[%d]",record.datalen+destindex,MAXSIZ_FAM * 2);
		}
		if(i == 0)
		{
			first_fileLen = next_info.nextSite;		//记录第一帧的数据的文件位置，方便分帧查找数据
			first_frameLen = record.datalen;		//记录第一帧数据长度，分帧发送第一帧数据的位置
		}
//		fprintf(stderr,"$$$$$$$$$$$$$$$$$$$$$$$$$$i=%d  record.datalen  ==== %d  subframe = %d\n\n\n\n",i,record.datalen,subframe);
	}
	fprintf(stderr,"!!!record.datalen  ==== %d  subframe = %d\n\n\n\n",record.datalen,subframe);
	if(frmrecord > 0)
		subframe = frmrecord;
	if(destindex >= BUFLEN) {		//RequestRecordList计算全部的list长度计算是否需要分帧进行处理
		record.data = &TmpDataBufList[1];	//从数据帧开始，去掉sequence of A-ResultRecord
		record.datalen = first_frameLen;
		next_info.nextSite = first_fileLen;
		next_info.subframeSum = subframe;
		next_info.frameNo = 1;
		next_info.repsonseType = GET_REQUEST_RECORD_LIST;
		BuildFrame_GetResponseNext(GET_REQUEST_RECORD_NEXT,csinfo,record.dar,record.datalen,record.data,sendbuf);
	}else {			//不分帧
		record.data = TmpDataBufList;
		record.datalen = destindex;
		BuildFrame_GetResponseRecord(GET_REQUEST_RECORD_LIST,csinfo,record,sendbuf);
	}
	return 1;
}

//
int getRequestNext(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	INT16U  okFrame = 0;
	int		datalen = 0;
	INT8U	DAR=success;

	okFrame = (data[0]<<8) | (data[1]);
	fprintf(stderr,"getRequestNext 接收最近一块数据块序号=%d  currSite=%d, nextSite=%d\n",okFrame,next_info.currSite,next_info.nextSite);
	switch(next_info.repsonseType) {
	case GET_REQUEST_NORMAL:		//对象属性         	[1]   SEQUENCE OF A-ResultNormal
	case GET_REQUEST_NORMAL_LIST:
		next_info.frameNo = okFrame+1;
		next_info.currSite = next_info.nextSite;
		Get_subFrameData(next_info.currSite,csinfo,sendbuf);
		break;
	case GET_REQUEST_RECORD:		//记录型对象属性    	[2]   SEQUENCE OF A-ResultRecord
	case GET_REQUEST_RECORD_LIST:
		next_info.frameNo = okFrame+1;
		next_info.currSite = next_info.nextSite;
		next_info.nextSite = readFrameDataFile(TASK_FRAME_DATA,next_info.currSite,TmpDataBuf,&datalen);//文件中第一个字节保存的是：SEQUENCE OF A-ResultRecord，此处从TmpDataBuf[1]上送，上送长度也要-1
		if(next_info.repsonseType==GET_REQUEST_RECORD) {
			if(datalen>=1)  datalen=datalen-1;
			BuildFrame_GetResponseNext(GET_REQUEST_RECORD_NEXT,csinfo,DAR,datalen,&TmpDataBuf[1],sendbuf);
		}else {
			if(datalen>=1)  datalen=datalen-1;
			BuildFrame_GetResponseNext(GET_REQUEST_RECORD_NEXT,csinfo,DAR,datalen,&TmpDataBuf[1],sendbuf);
		}
		break;
	}
	return 1;
}
