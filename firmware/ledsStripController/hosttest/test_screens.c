// host test: parameter decoding, dashboard lines, flash upgrade, warm-up timer and ECM watch. Run with run.sh
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "uds_parameters.h"
#include "giulia_watch.h"
uint8_t statistics_0_100_started=0, statistics_100_200_started=0, currentSchizzaforteMap='-', currentDNAmode=0;
void floatToStr(char* str, float num, uint8_t precision, uint8_t maxLen);
#include "extracted.inc"
static int fails=0;
static void pg(int page,const uint8_t*rx0,const uint8_t*rx1,float n0,float n1,const char*expect){
	float v[2]={NAN,NAN}; char s[64]; memset(s,0,sizeof s);
	const uint8_t*id=uds_params_array[0][page].udsParamId;
	if(rx0) udsDecodeReply(&single_uds_params_array[id[0]],rx0,8,&v[0]); else v[0]=n0;
	if(rx1) udsDecodeReply(&single_uds_params_array[id[1]],rx1,8,&v[1]); else v[1]=n1;
	if(id[0]==id[1]) v[1]=v[0];
	buildLineWithFormat(uds_params_array[0][page].name,v,id,s);
	int ok=strlen(uds_params_array[0][page].name)<=22 && strlen(s)<=18 && !strcmp(s,expect);
	if(!ok) fails++;
	printf("%s page %2d [%s]\n",ok?"ok  ":"FAIL",page,s);
}
#define B(...) (const uint8_t[]){__VA_ARGS__}
int main(void){
	pg(9, B(4,0x62,0x19,0x35,0x1E,0,0,0),0,0,0,"IC AirOut: -10  \xB0""C");
	pg(20,B(4,0x62,0x04,0xFE,0x20,0,0,0),0,0,0,"GEARBOX: -8   \xB0""C");
	pg(31,B(4,0x62,0x10,0x03,0x1E,0,0,0),0,0,0,"WATER: -10   \xB0""C ");
	pg(31,B(3,0x7F,0x22,0x31,0xAA,0xAA,0xAA,0xAA),0,0,0,"WATER:       \xB0""C ");
	pg(31,B(4,0x62,0x19,0x35,0x43,0,0,0),0,0,0,"WATER:       \xB0""C ");
	pg(32,B(5,0x62,0x18,0x41,0x05,0xDF,0,0),0,0,0,"KNOCK: 0.625   mV ");
	pg(47,B(4,0x62,0x04,0xFE,0x61,0,0,0),B(4,0x62,0x10,0x03,0x81,0,0,0),0,0,"GBX 57 \xB0""C W.89 \xB0""C");
	pg(47,B(4,0x62,0x04,0xFE,0x14,0,0,0),B(4,0x62,0x10,0x03,0x0A,0,0,0),0,0,"GBX -20\xB0""C W.-30\xB0""C");
	pg(48,B(5,0x62,0x1C,0x16,0x00,0x17,0,0),B(4,0x62,0x19,0x35,0x43,0,0,0),0,0,"CAC 23 \xB0""C IC 27 \xB0""C");
	pg(48,B(5,0x62,0x1C,0x16,0xFF,0xF4,0,0),B(4,0x62,0x19,0x35,0x1E,0,0,0),0,0,"CAC -12\xB0""C IC -10\xB0""C");
	pg(49,B(5,0x62,0x19,0x37,0x03,0x49,0,0),B(5,0x62,0x19,0x5A,0x04,0x12,0,0),0,0,"MAP 0.84 BST 1.04");
	pg(49,B(5,0x62,0x19,0x37,0x0A,0x1C,0,0),B(5,0x62,0x19,0x5A,0x0A,0x32,0,0),0,0,"MAP 2.59 BST 2.61");
	pg(50,B(5,0x62,0x3A,0x60,0x00,0x61,0,0),B(4,0x62,0x19,0x42,0x33,0,0,0),0,0,"AIR 12 kg 5.1 L/h");
	pg(50,B(5,0x62,0x3A,0x60,0x17,0x80,0,0),B(4,0x62,0x19,0x42,0xFF,0,0,0),0,0,"AIR 752kg 25.5L/h");
	pg(51,B(5,0x62,0x18,0x91,0x03,0x8C,0,0),B(5,0x62,0x18,0x92,0x03,0x8C,0,0),0,0,"KNK1 114 2 114mV");
	pg(52,B(5,0x62,0x18,0x93,0x0B,0x8A,0,0),B(5,0x62,0x18,0x94,0x0B,0x8A,0,0),0,0,"KNK3 369 4 369mV");
	pg(53,0,0,4,250,"GEAR 4 TQ 250Nm");
	pg(53,0,0,7,-35,"GEAR R TQ -35Nm");
	{ char b[6]; memset(b,'#',6); floatToStr(b,4294967040.0f,1,5); int ok=(b[5]=='#' && strlen(b)==4); if(!ok) fails++; printf("%s floatToStr wide number stays in buffer [%s]\n",ok?"ok  ":"FAIL",b); }
	{ char b[5]; floatToStr(b,1000.0f,1,4+1-1); int ok=!strcmp(b,"100"); if(!ok) fails++; printf("%s floatToStr 1000 in a 3 char field [%s]\n",ok?"ok  ":"FAIL",b); }
	{ char b[6]; floatToStr(b,100.0f,1,5); int ok=!strcmp(b,"100 "); if(!ok) fails++; printf("%s floatToStr 100.0 [%s]\n",ok?"ok  ":"FAIL",b); }
	{ char b[6]; floatToStr(b,-10.5f,1,5); int ok=!strcmp(b,"-10 "); if(!ok) fails++; printf("%s floatToStr -10.5 in $2.1f [%s]\n",ok?"ok  ":"FAIL",b); }
	{ const uint16_t want[5]={500,400,350,500,500}; const uint8_t mode[5]={0,1,2,3,255}; for(int k=0;k<5;k++){ int ok=screenRefreshPeriodMs(mode[k])==want[k] && screenRefreshPeriodMs(mode[k])>310; if(!ok) fails++; printf("%s refresh mode %d -> %d ms\n",ok?"ok  ":"FAIL",mode[k],screenRefreshPeriodMs(mode[k])); } }
	//pages 54..65
	pg(54,B(5,0x62,0x18,0x6C,0xFF,0xE8,0,0),B(5,0x62,0x18,0x6D,0x00,0x00,0,0),0,0,"SPK1 -1.5 2 0   \xB0");
	pg(54,B(5,0x62,0x18,0x6C,0xFF,0xFF,0,0),B(5,0x62,0x18,0x6D,0xFF,0x40,0,0),0,0,"SPK1 -0.1 2 -12 \xB0");
	pg(55,B(5,0x62,0x18,0x6E,0x00,0x30,0,0),B(5,0x62,0x18,0x6F,0xFF,0xD0,0,0),0,0,"SPK3 3    4 -3  \xB0");
	pg(56,B(5,0x62,0x18,0x41,0x05,0xDF,0,0),0,0,0,"KNOCK: 188 mV");
	pg(56,B(5,0x62,0x18,0x41,0x34,0xCB,0,0),0,0,0,"KNOCK: 1689mV");
	pg(57,B(5,0x62,0x19,0x46,0x52,0x88,0,0),B(5,0x62,0x19,0x47,0x53,0x8D,0,0),0,0,"RAIL 127/128bar");
	pg(57,B(5,0x62,0x19,0x46,0x84,0x61,0,0),B(5,0x62,0x19,0x47,0x1A,0x15,0,0),0,0,"RAIL 203/40 bar");
	pg(58,0,B(5,0x62,0x19,0x8E,0x07,0xA8,0,0),86,0,"OIL 86 \xB0""C MA 83 \xB0""C");
	pg(58,0,B(5,0x62,0x19,0x8E,0x01,0x40,0,0),-25,0,"OIL -25\xB0""C MA -20\xB0""C");
	pg(59,B(4,0x62,0x18,0xBA,0x7F,0,0,0),B(4,0x62,0x18,0x37,0x6C,0,0,0),0,0,"EGT585  CAT490 \xB0""C");
	pg(59,B(4,0x62,0x18,0xBA,0xD5,0,0,0),B(4,0x62,0x18,0x37,0xB7,0,0,0),0,0,"EGT1015 CAT865 \xB0""C");
	pg(60,B(4,0x62,0x18,0x6B,0x4B,0,0,0),B(5,0x62,0x10,0x00,0x0B,0xEC,0,0),0,0,"IDLE 750 /763 rpm");
	pg(60,B(4,0x62,0x18,0x6B,0x55,0,0,0),B(5,0x62,0x10,0x00,0x61,0xA8,0,0),0,0,"IDLE 850 /6250rpm");
	pg(61,B(3,0x41,0x06,0x83,0,0,0,0),B(3,0x41,0x07,0x7B,0,0,0,0),0,0,"ST 2.3   LT -3.9 %");
	pg(61,B(3,0x41,0x06,0x60,0,0,0,0),B(3,0x41,0x07,0xA0,0,0,0,0),0,0,"ST -25   LT 25   %");
	pg(61,B(3,0x41,0x07,0x83,0,0,0,0),B(4,0x62,0x01,0x07,0x7B,0,0,0),0,0,"ST       LT      %");
	pg(61,B(3,0x7F,0x01,0x12,0,0,0,0),B(2,0x41,0x07,0x7B,0,0,0,0),0,0,"ST       LT      %");
	pg(62,B(3,0x41,0x46,0x34,0,0,0,0),B(4,0x62,0x10,0x03,0x7C,0,0,0),0,0,"OUT 12 \xB0""C W.84 \xB0""C");
	pg(62,B(3,0x41,0x46,0x10,0,0,0,0),B(4,0x62,0x10,0x03,0x0A,0,0,0),0,0,"OUT -24\xB0""C W.-30\xB0""C");
	pg(62,B(4,0x62,0x10,0x03,0x7C,0,0,0),B(3,0x41,0x46,0x34,0,0,0,0),0,0,"OUT    \xB0""C W.   \xB0""C");
	pg(63,B(3,0x41,0x0E,0x8B,0,0,0,0),B(3,0x41,0x04,0x3B,0,0,0,0),0,0,"ADV 5.5  \xB0"" LD 23 %");
	pg(63,B(3,0x41,0x0E,0x6C,0,0,0,0),B(3,0x41,0x04,0xFF,0,0,0,0),0,0,"ADV -10  \xB0"" LD 100%");
	pg(64,B(4,0x41,0x44,0x80,0xE1,0,0,0),0,0,0,"LAMBDA: 1.007");
	pg(64,B(4,0x41,0x44,0x69,0x00,0,0,0),0,0,0,"LAMBDA: 0.82 ");
	pg(65,0,0,12.5,80,"W.UP 12.5min 80 \xB0""C");
	pg(65,0,0,0,-25,"W.UP 0   min -25\xB0""C");
	pg(27,B(5,0x62,0x20,0x06,0x01,0x2C,0,0),0,0,0,"OVER RPM: 3  sec");
	{ int ok=single_uds_params_array[38].replyId==0x18DAF110; if(!ok) fails++; printf("%s record 38 replyId\n",ok?"ok  ":"FAIL"); }
	{ int bad=0; for(int f=0;f<2;f++){ int tot=f?total_pages_in_dashboard_menu_diesel:total_pages_in_dashboard_menu_gasoline; if(tot>UDS_PAGES_MAX) bad++;
		for(int pgn=0;pgn<tot;pgn++){ const uds_params_couple_element*e=&uds_params_array[f][pgn]; if(!e->name[0] || strlen(e->name)>22) bad++;
			for(int k=0;k<2;k++){ if(e->udsParamId[k]>=UDS_SINGLE_PARAMS_MAX || single_uds_params_array[e->udsParamId[k]].reqId==0) bad++; } } }
	  if(bad) fails++; printf("%s tables: every page has a name <=22 and filled records (bad=%d)\n",bad?"FAIL":"ok  ",bad); }
	{ //warm-up timer: 10 ms frames, simulated clock
	  uint32_t t=1000; int ok=1; float m;
	  for(int k=0;k<500;k++){ warmupTimerUpdate(t,0,60); t+=10; }               //ignition on, engine off
	  ok&=(warmupTimerMinutes()==0.0f);
	  for(int k=0;k<75000;k++){ warmupTimerUpdate(t,900,60+k/1500); t+=10; }    //12.5 min running, oil from 20 to 69 C
	  m=warmupTimerMinutes(); ok&=(m>12.39f && m<12.51f);
	  for(int k=0;k<3000;k++){ warmupTimerUpdate(t,0,110); t+=10; }             //30 s stop: no restart
	  for(int k=0;k<6000;k++){ warmupTimerUpdate(t,900,121); t+=10; }           //oil 81 C: number freezes at the first frame
	  m=warmupTimerMinutes(); ok&=(m>12.99f && m<13.11f);
	  for(int k=0;k<60000;k++){ warmupTimerUpdate(t,2000,100); t+=10; }         //oil falls to 60 C while driving: stays frozen
	  ok&=(warmupTimerMinutes()==m);
	  for(int k=0;k<12000;k++){ warmupTimerUpdate(t,0,125); t+=10; }            //2 min stop, warm oil
	  for(int k=0;k<1000;k++){ warmupTimerUpdate(t,900,125); t+=10; }           //warm restart keeps the result
	  ok&=(warmupTimerMinutes()==m);
	  for(int k=0;k<12000;k++){ warmupTimerUpdate(t,0,50); t+=10; }             //2 min stop, cold oil
	  for(int k=0;k<6100;k++){ warmupTimerUpdate(t,900,50); t+=10; }            //cold restart: new count, 1.0 min
	  float m2=warmupTimerMinutes(); ok&=(m2>0.99f && m2<1.11f);
	  for(int k=0;k<12000;k++){ warmupTimerUpdate(t,0,50); t+=10; }             //aborted warm-up, 2 min stop
	  for(int k=0;k<10;k++){ warmupTimerUpdate(t,900,50); t+=10; }              //restart from zero
	  ok&=(warmupTimerMinutes()==0.0f);
	  for(int k=0;k<660000;k++){ warmupTimerUpdate(t,900,50); t+=10; }          //110 min cold: capped at 99.9
	  ok&=(warmupTimerMinutes()==99.9f);
	  if(!ok) fails++; printf("%s warm-up timer (%.1f, %.1f)\n",ok?"ok  ":"FAIL",m,m2); }

	//ECM watch and consumption
	#define CK(name,cond) do{ int ok=(cond); if(!ok) fails++; printf("%s %s\n",ok?"ok  ":"FAIL",name); }while(0)
	#define RX(...) gwOnReply(&g,B(__VA_ARGS__),8,now,spd,cold)
	{ gw_state g; char t[19]; uint32_t now=1000; float spd=0; uint8_t cold=75; uint8_t d[8];
	  for(int mode=0;mode<4;mode++){ gwReset(&g,0); int n[4]={0,0,0,0},bad=0; uint32_t last=0; int first=1;
	    for(uint32_t ms=1;ms<=60000;ms++){ uint8_t it=gwNextItem(&g,ms,(uint8_t)mode); if(it!=GW_ITEM_NONE){ n[it]++; if(!first && ms-last<GW_SLOT_MS) bad=1; last=ms; first=0; } }
	    printf("     mode %d in 60 s: cac %d water %d ic %d dtc %d\n",mode,n[0],n[1],n[2],n[3]);
	    CK("scheduler: at most one request per second", !bad && n[0]+n[1]+n[2]+n[3]<=60);
	    CK("scheduler: mode selects the items", (mode==0 && n[0]+n[1]+n[2]+n[3]==0) || (mode==1 && n[3]==6 && n[0]+n[1]+n[2]==54) || (mode==2 && n[3]==0 && n[0]+n[1]+n[2]==60) || (mode==3 && n[3]==6 && n[0]+n[1]+n[2]==0)); }
	  gwReset(&g,0);
	  CK("request 22 1003", gwBuildRequest(&g,GW_ITEM_WATER,d)==4 && !memcmp(d,B(3,0x22,0x10,0x03),4));
	  CK("request 22 1C16", gwBuildRequest(&g,GW_ITEM_CAC,d)==4 && !memcmp(d,B(3,0x22,0x1C,0x16),4));
	  CK("request 19 01 0D", gwBuildRequest(&g,GW_ITEM_DTC,d)==4 && !memcmp(d,B(3,0x19,0x01,0x0D),4));
	  //faults
	  CK("dtc: first count is the baseline, no alarm", RX(6,0x59,0x01,0xFF,0x01,0x00,0x03,0)==0 && g.dtcCount==3 && g.dtcBase==3);
	  CK("dtc: same count, no alarm", RX(6,0x59,0x01,0xFF,0x01,0x00,0x03,0)==0);
	  CK("dtc: count grows, alarm", RX(6,0x59,0x01,0xFF,0x01,0x00,0x04,0)==GW_AL_NEW_FAULT && gwAlarmText(&g,GW_AL_NEW_FAULT,t)==16 && !strcmp(t,"NEW ECM FAULT: 1"));
	  CK("dtc: grows again, second alarm", RX(6,0x59,0x01,0xFF,0x01,0x00,0x06,0)==GW_AL_NEW_FAULT && gwAlarmText(&g,GW_AL_NEW_FAULT,t) && !strcmp(t,"NEW ECM FAULT: 3"));
	  CK("dtc: count falls, no alarm, new baseline", RX(6,0x59,0x01,0xFF,0x01,0x00,0x02,0)==0 && g.dtcBase==2);
	  CK("dtc: multi frame 59 02 is ignored", RX(0x10,0x0B,0x59,0x02,0xFF,0x00,0xE1,0x00)==0 && g.dtcCount==2);
	  CK("dtc: short answer is ignored", gwOnReply(&g,B(3,0x59,0x01,0xFF,0,0,0,0),4,now,spd,cold)==0 && g.dtcCount==2);
	  CK("dtc: 7F 22 31 changes nothing", RX(3,0x7F,0x22,0x31,0,0,0,0)==0 && g.dtcNrc==0);
	  CK("dtc: responsePending is not a refusal", RX(3,0x7F,0x19,0x78,0,0,0,0)==0 && g.dtcNrc==0);
	  RX(3,0x7F,0x19,0x12,0,0,0,0); RX(3,0x7F,0x19,0x12,0,0,0,0); RX(3,0x7F,0x19,0x12,0,0,0,0);
	  CK("dtc: 3 refusals switch the mask to 08", g.dtcMask==0x08 && !g.dtcOff && gwBuildRequest(&g,GW_ITEM_DTC,d) && d[3]==0x08);
	  RX(3,0x7F,0x19,0x12,0,0,0,0); RX(3,0x7F,0x19,0x12,0,0,0,0); RX(3,0x7F,0x19,0x12,0,0,0,0);
	  { int asked=0; for(uint32_t ms=now;ms<now+60000;ms++) if(gwNextItem(&g,ms,3)==GW_ITEM_DTC) asked++; CK("dtc: 3 more refusals stop the requests", g.dtcOff && asked==0); }
	  g.dtcCount=1001; g.dtcBase=0; CK("dtc: text stays in 18 chars", gwAlarmText(&g,GW_AL_NEW_FAULT,t)==18 && !strcmp(t,"NEW ECM FAULT: 999"));
	  //coolant hot
	  gwReset(&g,0); now=5000;
	  CK("hot: 107 no alarm", RX(4,0x62,0x10,0x03,107+40,0,0,0)==0);
	  CK("hot: first 108 no alarm", RX(4,0x62,0x10,0x03,108+40,0,0,0)==0);
	  { uint8_t r=0; for(int k=0;k<12;k++){ now+=400; r|=RX(4,0x62,0x10,0x03,108+40,0,0,0);} CK("hot: 12 fast samples in 4.8 s are not enough", r==0); }
	  now+=200; CK("hot: alarm after 5 s", RX(4,0x62,0x10,0x03,108+40,0,0,0)==GW_AL_COOLANT_HOT && gwAlarmText(&g,GW_AL_COOLANT_HOT,t)==17 && !strcmp(t,"COOLANT HOT 108\xB0""C"));
	  CK("hot: not raised twice", RX(4,0x62,0x10,0x03,112+40,0,0,0)==0);
	  CK("hot: 105 keeps it active", RX(4,0x62,0x10,0x03,105+40,0,0,0)==0 && (g.active&GW_AL_COOLANT_HOT));
	  CK("hot: 104 clears", RX(4,0x62,0x10,0x03,104+40,0,0,0)==0 && !(g.active&GW_AL_COOLANT_HOT));
	  RX(4,0x62,0x10,0x03,0xFF,0,0,0); now+=5000; RX(4,0x62,0x10,0x03,0xFF,0,0,0);
	  CK("hot: text with the largest value", gwAlarmText(&g,GW_AL_COOLANT_HOT,t)==17 && !strcmp(t,"COOLANT HOT 215\xB0""C"));
	  //coolant cold
	  gwReset(&g,0); spd=110; now=19*60*1000; { uint8_t r=0; for(int k=0;k<5;k++){ now+=4000; r|=RX(4,0x62,0x10,0x03,74+40,0,0,0);} CK("cold: nothing before 20 min", r==0); }
	  now=21*60*1000; { uint8_t r=0; r|=RX(4,0x62,0x10,0x03,74+40,0,0,0); now+=4000; r|=RX(4,0x62,0x10,0x03,74+40,0,0,0); CK("cold: 4 s are not enough", r==0); now+=4000; r=RX(4,0x62,0x10,0x03,74+40,0,0,0); CK("cold: 8 s are not enough", r==0); now+=4000; r=RX(4,0x62,0x10,0x03,74+40,0,0,0); CK("cold: alarm after 10 s", r==GW_AL_COOLANT_COLD && gwAlarmText(&g,GW_AL_COOLANT_COLD,t)==17 && !strcmp(t,"COOLANT COLD 74\xB0""C")); }
	  CK("cold: 77 keeps it active", RX(4,0x62,0x10,0x03,77+40,0,0,0)==0 && (g.active&GW_AL_COOLANT_COLD));
	  CK("cold: 78 clears", RX(4,0x62,0x10,0x03,78+40,0,0,0)==0 && !(g.active&GW_AL_COOLANT_COLD));
	  spd=60; RX(4,0x62,0x10,0x03,74+40,0,0,0); spd=110; { uint8_t r=0; for(int k=0;k<20;k++){ now+=4000; r|=RX(4,0x62,0x10,0x03,74+40,0,0,0);} CK("cold: 80 s after a slow stretch, no alarm", r==0); for(int k=0;k<15;k++){ now+=4000; r|=RX(4,0x62,0x10,0x03,74+40,0,0,0);} CK("cold: alarm again after 120 s above 80 km/h", r==GW_AL_COOLANT_COLD); }
	  gwReset(&g,0); now=30*60*1000; cold=0; { uint8_t r=0; for(int k=0;k<10;k++){ now+=4000; r|=RX(4,0x62,0x10,0x03,60+40,0,0,0);} CK("cold: limit off, no alarm", r==0); } cold=75;
	  //intercooler air
	  gwReset(&g,0); spd=0; { uint8_t r=0; for(int k=0;k<6;k++) r|=RX(4,0x62,0x19,0x35,72+40,0,0,0); CK("ic air: hot at standstill, no alarm", r==0); }
	  spd=50; { uint8_t r=0; for(int k=0;k<3;k++){ r|=RX(4,0x62,0x19,0x35,70+40,0,0,0); now+=4000; } CK("ic air: 8 s moving are not enough", r==0); }
	  CK("ic air: alarm after 10 s", RX(4,0x62,0x19,0x35,70+40,0,0,0)==GW_AL_ICAIR_HOT && gwAlarmText(&g,GW_AL_ICAIR_HOT,t)==15 && !strcmp(t,"IC AIR HOT 70\xB0""C"));
	  CK("ic air: 58 clears", RX(4,0x62,0x19,0x35,58+40,0,0,0)==0 && !(g.active&GW_AL_ICAIR_HOT));
	  //a gap in the samples restarts the hold
	  gwReset(&g,0); now=1000; spd=0; RX(4,0x62,0x10,0x03,110+40,0,0,0); now+=60000;
	  CK("hold: a 60 s gap does not count", RX(4,0x62,0x10,0x03,110+40,0,0,0)==0);
	  //Start&Stop keeps the state, a long stop resets it
	  gwReset(&g,0); now=25*60*1000; RX(6,0x59,0x01,0xFF,0x01,0x00,0x03,0);
	  CK("resume: 40 s stop keeps run time and baseline", gwEngineStart(&g,now+40000,now,1)==0 && g.startMs==0 && g.dtcBase==3 && (g.have&(1<<GW_ITEM_DTC)));
	  CK("resume: 3 min stop resets", gwEngineStart(&g,now+180000,now,1)==1 && g.startMs==(uint32_t)(now+180000) && !(g.have&(1<<GW_ITEM_DTC)));
	  CK("resume: first start resets", gwEngineStart(&g,5000,0,0)==1 && g.startMs==5000);
	#if GW_CAC_VERIFIED
	  //charge air cooler sensor
	  gwReset(&g,0); now=1000; RX(4,0x62,0x10,0x03,90+40,0,0,0);
	  CK("cac: first value, no alarm", RX(5,0x62,0x1C,0x16,0x00,25,0,0)==0); now+=2000;
	  CK("cac: +1, no alarm", RX(5,0x62,0x1C,0x16,0x00,26,0,0)==0); now+=2000;
	  CK("cac: drop to -40, alarm", RX(5,0x62,0x1C,0x16,0xFF,0xD8,0,0)==GW_AL_CAC_DROP && g.cacDrops==1 && g.cacMin==-40 && gwAlarmText(&g,GW_AL_CAC_DROP,t)==15 && !strcmp(t,"CAC SENSOR DROP")); now+=2000;
	  CK("cac: jump back is the same event", RX(5,0x62,0x1C,0x16,0x00,26,0,0)==0 && g.cacDrops==1 && (g.active&GW_AL_CAC_DROP)); now+=2000;
	  CK("cac: 2 close samples clear", RX(5,0x62,0x1C,0x16,0x00,26,0,0)==0 && !(g.active&GW_AL_CAC_DROP)); now+=2000;
	  CK("cac: second drop is counted", RX(5,0x62,0x1C,0x16,0x7F,0xFF,0,0)==GW_AL_CAC_DROP && g.cacDrops==2);
	  gwReset(&g,0); now=1000; RX(4,0x62,0x10,0x03,2,0,0,0);
	  CK("cac: -38 on a -38 engine, no alarm", RX(5,0x62,0x1C,0x16,0xFF,0xDA,0,0)==0 && g.cacDrops==0);
	  gwReset(&g,0); now=1000; RX(5,0x62,0x1C,0x16,0x00,25,0,0); now+=30000;
	  CK("cac: old sample is not compared", RX(5,0x62,0x1C,0x16,0x00,60,0,0)==0);
	  CK("cac: DLC 5 is ignored", gwOnReply(&g,B(5,0x62,0x1C,0x16,0xFF,0,0,0),5,now,spd,cold)==0 && g.cac==60);
	#else
	  gwReset(&g,0); now=1000; RX(4,0x62,0x10,0x03,90+40,0,0,0); RX(5,0x62,0x1C,0x16,0x00,25,0,0); now+=2000;
	  CK("cac: watch is off while 1C16 is not verified", RX(5,0x62,0x1C,0x16,0x7F,0xFF,0,0)==0 && g.cacDrops==0 && g.active==0);
	#endif
	  //consumption
	  CK("L/100: 8.4 L/h at 100 km/h", fabsf(gwFuelPer100(8.4f,100)-8.4f)<0.001f);
	  CK("L/100: fuel cut is 0", gwFuelPer100(0,80)==0);
	  CK("L/100: below 5 km/h is blank", isnan(gwFuelPer100(1.0f,4.9f)) && isnan(gwFuelPer100(NAN,100)));
	  CK("L/100: capped at 99.9", gwFuelPer100(25.5f,10)==99.9f);
	}
	pg(66,B(4,0x62,0x19,0x42,0x54,0,0,0),0,0,8.4f,"8.4 L/h 8.4 L/100");
	pg(66,B(4,0x62,0x19,0x42,0x0A,0,0,0),0,0,NAN,"1   L/h     L/100");
	pg(66,B(4,0x62,0x19,0x42,0xFF,0,0,0),0,0,99.9f,"25.5L/h 99.9L/100");
	{ uint8_t sh[240]; int ok;
	  memset(sh,0,sizeof sh); sh[0]=1; shownParamsUpgrade(sh,0x1234,0); ok=sh[0]==1 && sh[1]==0 && sh[46]==0 && sh[47]==1 && sh[60]==1 && sh[61]==0 && sh[64]==0 && sh[65]==1 && sh[66]==1;
	  if(!ok) fails++; printf("%s upgrade: new gasoline pages shown, OBD pages hidden, old choice kept\n",ok?"ok  ":"FAIL");
	  memset(sh,0,sizeof sh); shownParamsUpgrade(sh,SHOWN_PARAMS_MARK,0); ok=sh[47]==0 && sh[66]==0; if(!ok) fails++; printf("%s upgrade: with the mark nothing changes\n",ok?"ok  ":"FAIL");
	  memset(sh,0,sizeof sh); shownParamsUpgrade(sh,0xFFFF,1); ok=sh[47]==0; if(!ok) fails++; printf("%s upgrade: diesel untouched\n",ok?"ok  ":"FAIL"); }
	printf("fails=%d\n",fails); return fails;
}
