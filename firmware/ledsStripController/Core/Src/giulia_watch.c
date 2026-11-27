//giulia h4: see giulia_watch.h
#include "giulia_watch.h"
#include <math.h>
#include <string.h>

static const uint16_t gwDid[3]={0x1C16,0x1003,0x1935};

void gwReset(gw_state *s, uint32_t now){
	memset(s,0,sizeof(*s));
	s->startMs=now; s->lastPollMs=now; s->lastDtcMs=now-GW_DTC_PERIOD_MS; s->slowSinceMs=now;
	s->cacMin=32767; s->dtcMask=GW_DTC_MASK;
}

//1 once cond has held for holdMs. A gap in the samples (older than GW_STALE_MS) restarts the count
static uint8_t held(uint8_t cond, uint8_t *running, uint32_t *since, uint32_t lastSample, uint32_t now, uint32_t holdMs){
	if(!cond){ *running=0; return 0; }
	if(!*running || now-lastSample>GW_STALE_MS){ *running=1; *since=now; }
	return (now-*since)>=holdMs;
}

//engine (re)started: after a short stop the run time, the fault baseline and the minima are kept
uint8_t gwEngineStart(gw_state *s, uint32_t now, uint32_t stopMs, uint8_t hadRun){
	if(!hadRun || now-stopMs>=GW_RESUME_MS){ gwReset(s,now); return 1; }
	s->hotN=0; s->coldN=0; s->icN=0;
	s->lastPollMs=now; s->slowSinceMs=now;
	return 0;
}

//returns the item to ask now, or GW_ITEM_NONE. The caller checks engine, guard window and the other UDS users.
uint8_t gwNextItem(gw_state *s, uint32_t now, uint8_t mode){
	if(now-s->lastPollMs<GW_SLOT_MS) return GW_ITEM_NONE;
	uint8_t item=GW_ITEM_NONE;
	if(GW_MODE_HAS_DTC(mode) && !s->dtcOff && (now-s->lastDtcMs>=GW_DTC_PERIOD_MS)){
		item=GW_ITEM_DTC; s->lastDtcMs=now;
	}else if(GW_MODE_HAS_TEMP(mode)){
		#if GW_CAC_VERIFIED
			static const uint8_t seq[4]={GW_ITEM_CAC,GW_ITEM_WATER,GW_ITEM_CAC,GW_ITEM_ICAIR};
		#else
			static const uint8_t seq[4]={GW_ITEM_WATER,GW_ITEM_ICAIR,GW_ITEM_WATER,GW_ITEM_ICAIR};
		#endif
		item=seq[(s->slot++)&3];
	}
	if(item!=GW_ITEM_NONE) s->lastPollMs=now;
	return item;
}

//fills a single frame request, returns the DLC
uint8_t gwBuildRequest(const gw_state *s, uint8_t item, uint8_t d[8]){
	memset(d,0,8);
	d[0]=0x03;
	if(item==GW_ITEM_DTC){ d[1]=0x19; d[2]=0x01; d[3]=s->dtcMask; }
	else{ d[1]=0x22; d[2]=(uint8_t)(gwDid[item]>>8); d[3]=(uint8_t)gwDid[item]; }
	return 4;
}

uint8_t gwColdLimit(uint8_t v){ //flash slot 39: 0 = 75 (default), 1 = 70, 2 = 80, 3 = off
	return v==1 ? 70 : (v==2 ? 80 : (v==3 ? 0 : 75));
}

static uint8_t raise(gw_state *s, uint8_t bit){ if(s->active&bit) return 0; s->active|=bit; return bit; }

//takes every frame from 0x18DAF110. Returns the alarms raised by this frame (GW_AL_x), 0 otherwise.
uint8_t gwOnReply(gw_state *s, const uint8_t *rx, uint8_t dlc, uint32_t now, float speed, uint8_t coldLimit){
	uint8_t out=0;
	if(dlc<4 || (rx[0]&0xF0)!=0) return 0; //single frames only
	if(speed<GW_COLD_SPEED) s->slowSinceMs=now;

	if(rx[1]==0x7F && rx[2]==0x19){ //19 01 refused
		if(rx[3]==0x78 || rx[3]==0x21) return 0; //pending or busy: ask again later
		if(++s->dtcNrc>=GW_DTC_NRC_LIMIT){
			if(s->dtcMask!=0x08){ s->dtcMask=0x08; s->dtcNrc=0; } //second try: confirmed only
			else s->dtcOff=1;
		}
		return 0;
	}
	if(rx[1]==0x59 && rx[2]==0x01){
		if(dlc<7 || rx[0]<6) return 0;
		uint16_t n=((uint16_t)rx[5]<<8)|rx[6];
		s->dtcNrc=0;
		if(!(s->have&(1<<GW_ITEM_DTC))){ s->have|=(1<<GW_ITEM_DTC); s->dtcBase=n; } //first answer: what is stored is not news
		else if(n>s->dtcCount){ s->active&=(uint8_t)~GW_AL_NEW_FAULT; out|=raise(s,GW_AL_NEW_FAULT); }
		if(n<s->dtcBase) s->dtcBase=n; //faults cleared or healed
		s->dtcCount=n;
		return out;
	}
	if(rx[1]!=0x62) return 0;
	uint16_t did=((uint16_t)rx[2]<<8)|rx[3];

	if(did==0x1003 && dlc>=5){
		uint32_t prev=s->tWater;
		s->water=(int16_t)rx[4]-40; s->tWater=now; s->have|=(1<<GW_ITEM_WATER);
		if(held(s->water>=GW_COOLANT_HOT_ON, &s->hotN, &s->hotSince, prev, now, GW_HOT_HOLD_MS)) out|=raise(s,GW_AL_COOLANT_HOT);
		if(s->water<=GW_COOLANT_HOT_OFF) s->active&=(uint8_t)~GW_AL_COOLANT_HOT;
		uint8_t coldNow= coldLimit && (now-s->startMs>=GW_COLD_RUN_MS) && (now-s->slowSinceMs>=GW_COLD_FAST_MS) && (s->water<(int16_t)coldLimit);
		if(held(coldNow, &s->coldN, &s->coldSince, prev, now, GW_COLD_HOLD_MS)) out|=raise(s,GW_AL_COOLANT_COLD);
		if(!coldLimit || s->water>=(int16_t)coldLimit+3) s->active&=(uint8_t)~GW_AL_COOLANT_COLD;
	}else if(did==0x1935 && dlc>=5){
		uint32_t prev=s->tIc;
		s->icAir=(int16_t)rx[4]-40; s->tIc=now; s->have|=(1<<GW_ITEM_ICAIR);
		if(held(s->icAir>=GW_ICAIR_HOT_ON && speed>=GW_ICAIR_SPEED, &s->icN, &s->icSince, prev, now, GW_ICAIR_HOLD_MS)) out|=raise(s,GW_AL_ICAIR_HOT);
		if(s->icAir<=GW_ICAIR_HOT_OFF) s->active&=(uint8_t)~GW_AL_ICAIR_HOT;
	}else if(did==0x1C16 && dlc>=6){
		int16_t v=(int16_t)(((uint16_t)rx[4]<<8)|rx[5]);
		uint8_t had=s->have&(1<<GW_ITEM_CAC);
		uint8_t fresh= had && (now-s->tCac<=GW_STALE_MS);
		int16_t d=(int16_t)(v-s->cac); if(d<0) d=(int16_t)-d;
		uint8_t warm=(s->have&(1<<GW_ITEM_WATER)) && s->water>=40;
		uint8_t bad= (fresh && d>=GW_CAC_JUMP) || (v<=GW_CAC_LOW && warm) || v>=GW_CAC_HIGH;
		if(bad){
			if(!(s->active&GW_AL_CAC_DROP) && s->cacDrops<255) s->cacDrops++;
			out|=raise(s,GW_AL_CAC_DROP);
		}else if(fresh && d<5){
			s->active&=(uint8_t)~GW_AL_CAC_DROP; //two close samples in a row: the signal is back
		}
		if(v<s->cacMin) s->cacMin=v;
		s->cac=v; s->tCac=now; s->have|=(1<<GW_ITEM_CAC);
	}
	return out;
}

static uint8_t putInt(char *o, int v){
	char t[7]; uint8_t n=0, k=0;
	if(v<0){ o[k++]='-'; v=-v; }
	do{ t[n++]=(char)('0'+v%10); v/=10; }while(v && n<6);
	while(n) o[k++]=t[--n];
	return k;
}

//writes the warning line (max 18 chars, zero terminated), returns its length
uint8_t gwAlarmText(const gw_state *s, uint8_t alarm, char out[19]){
	const char *head=""; int val=0; uint8_t deg=1, num=1;
	switch(alarm){
		case GW_AL_COOLANT_HOT:  head="COOLANT HOT ";  val=s->water; break;
		case GW_AL_COOLANT_COLD: head="COOLANT COLD "; val=s->water; break;
		case GW_AL_ICAIR_HOT:    head="IC AIR HOT ";   val=s->icAir; break;
		case GW_AL_CAC_DROP:     head="CAC SENSOR DROP"; num=0; break;
		case GW_AL_NEW_FAULT:    head="NEW ECM FAULT: "; val=(int)s->dtcCount-(int)s->dtcBase; if(val<1) val=1; if(val>999) val=999; deg=0; break;
		default: out[0]=0; return 0;
	}
	if(val>999) val=999;
	if(val<-99) val=-99;
	uint8_t k=(uint8_t)strlen(head);
	memcpy(out,head,k);
	if(num){
		k+=putInt(&out[k],val);
		if(deg){ out[k++]=(char)0xB0; out[k++]='C'; }
	}
	out[k]=0;
	return k;
}

//L/100 km from L/h and km/h. NAN below 5 km/h, capped at 99.9
float gwFuelPer100(float lph, float speed){
	if(lph!=lph || speed<5.0f) return NAN;
	float v=lph*100.0f/speed;
	return v>99.9f ? 99.9f : v;
}
