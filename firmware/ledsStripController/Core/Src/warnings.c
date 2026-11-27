//see warnings.h
#include <string.h>
#include "warnings.h"

typedef struct {
	uint8_t  active;
	uint8_t  flags;
	uint8_t  chimes;		//chimes asked since the raise
	uint8_t  chimeDue;
	uint32_t lastChime;		//0 = never
	char     text[WARN_TEXT_LEN];
} warn_slot;

static warn_slot warnTab[WARN_SLOTS];
static uint32_t warnWindowStart;	//when the overlay in progress (or the last one) began
static uint8_t  warnWindowOpen;
static uint8_t  warnShown=0xFF;		//slot shown in the window
static uint8_t  warnEverShown;
static uint32_t warnLastChimeAny;
static uint8_t  warnChimedOnce;

void warnReset(void){
	memset(warnTab,0,sizeof(warnTab));
	warnWindowOpen=0; warnShown=0xFF; warnEverShown=0; warnChimedOnce=0;
}

void warnSet(uint8_t id, uint8_t active, uint8_t flags, const char *text, uint32_t now){
	if(id>=WARN_SLOTS) return;
	warn_slot *w=&warnTab[id];
	if(active){
		uint8_t n=0;
		while(n<WARN_TEXT_LEN && text[n]){ w->text[n]=text[n]; n++; }
		while(n<WARN_TEXT_LEN) w->text[n++]=' ';
		w->flags=flags;
		if(!w->active){ //raised now
			w->active=1;
			w->chimes=0;
			if((flags & WARN_F_CHIME) && (w->lastChime==0 || now-w->lastChime>=WARN_CHIME_REARM_MS)) w->chimeDue=1;
		}else if((flags & WARN_F_CRITICAL) && (flags & WARN_F_CHIME) && w->chimes>0 && w->chimes<WARN_CHIME_REPEAT_MAX && now-w->lastChime>=WARN_CHIME_REPEAT_MS){
			w->chimeDue=1;
		}
	}else{
		w->active=0;
		w->chimeDue=0;
	}
}

uint8_t warnCriticalActive(void){
	for(uint8_t i=0;i<WARN_SLOTS;i++) if(warnTab[i].active && (warnTab[i].flags & WARN_F_CRITICAL)) return 1;
	return 0;
}

uint8_t warnOverlay(uint32_t now, uint8_t *dst18){
	uint8_t any=0;
	for(uint8_t i=0;i<WARN_SLOTS;i++) if(warnTab[i].active) any=1;
	if(!any){ warnWindowOpen=0; return 0; }
	if(warnWindowOpen){
		if(now-warnWindowStart>=WARN_SHOW_MS || warnShown>=WARN_SLOTS || !warnTab[warnShown].active) warnWindowOpen=0;
	}
	if(!warnWindowOpen){
		uint32_t period=warnCriticalActive() ? WARN_PERIOD_CRIT_MS : WARN_PERIOD_MS;
		if(warnEverShown && now-warnWindowStart<period) return 0;
		//next slot: a critical one always wins, the others take turns
		uint8_t pick=0xFF;
		for(uint8_t i=0;i<WARN_SLOTS && pick==0xFF;i++) if(warnTab[i].active && (warnTab[i].flags & WARN_F_CRITICAL)) pick=i;
		for(uint8_t k=1;k<=WARN_SLOTS && pick==0xFF;k++){
			uint8_t i=(uint8_t)((warnShown>=WARN_SLOTS ? WARN_SLOTS-1 : warnShown)+k)%WARN_SLOTS;
			if(warnTab[i].active) pick=i;
		}
		warnShown=pick;
		warnWindowStart=now;
		warnWindowOpen=1;
		warnEverShown=1;
	}
	memcpy(dst18,warnTab[warnShown].text,WARN_TEXT_LEN);
	return 1;
}

uint8_t warnChimePending(uint32_t now){
	if(warnChimedOnce && now-warnLastChimeAny<WARN_CHIME_GAP_MS) return 0;
	for(uint8_t i=0;i<WARN_SLOTS;i++){
		warn_slot *w=&warnTab[i];
		if(w->active && w->chimeDue){
			w->chimeDue=0;
			w->chimes++;
			w->lastChime=now ? now : 1;
			warnLastChimeAny=now;
			warnChimedOnce=1;
			return 1;
		}
	}
	return 0;
}

//oil warnings from the native 0x4B2 values
void oilWarnEvaluate(uint32_t now, uint32_t rpm, uint16_t engineOnTicks, uint8_t oilTraw, uint8_t oilPraw, uint32_t oilFrameAge){
	static uint32_t coldSince, coldRaisedAt, lowSince;
	static uint8_t coldCond, coldOn, lowCond, lowOn;
	uint8_t valid=(engineOnTicks>=WARN_ENGINE_ON_TICKS) && (oilFrameAge<WARN_OIL_STALE_MS);

	//cold oil and high rpm
	uint8_t c=valid && oilTraw!=0xFF && oilTraw<WARN_OILT_COLD_RAW && rpm>(coldOn ? WARN_OILT_RPM_OFF : WARN_OILT_RPM_ON);
	if(c && !coldCond) coldSince=now;
	coldCond=c;
	if(!coldOn){
		if(c && now-coldSince>=WARN_OILT_ON_MS){ coldOn=1; coldRaisedAt=now; }
	}else if(!c && now-coldRaisedAt>=WARN_OILT_MIN_MS){
		coldOn=0;
	}
	if(!valid) coldOn=0;
	warnSet(WARN_ID_OIL_COLD, coldOn, WARN_F_CHIME, "OIL COLD: EASY RPM", now);

	//low oil pressure
	uint8_t l=valid && oilPraw!=0xFF && rpm>=WARN_OILP_RPM && oilPraw<(lowOn ? WARN_OILP_CLEAR_RAW : WARN_OILP_LOW_RAW);
	if(l && !lowCond) lowSince=now;
	lowCond=l;
	if(!lowOn){
		if(l && now-lowSince>=WARN_OILP_ON_MS) lowOn=1;
	}else if(!l){
		lowOn=0;
	}
	warnSet(WARN_ID_OIL_PRESSURE, lowOn, WARN_F_CRITICAL|WARN_F_CHIME, "OIL PRESSURE LOW", now);
}

//drive style (0x384 code) to index
uint8_t dnaModeIndex(uint8_t mode384){
	switch(mode384){
		case 0x00: return 0;
		case 0x08: return 1;
		case 0x10: return 2;
		case 0x30: return 3;
		default: return 0xFF;
	}
}

//each slot keeps two bytes, page+1. 0 (slot never written, or written as 0) and anything out of range mean not bound
uint8_t dnaPageGet(uint16_t slotND, uint16_t slotAR, uint8_t idx, uint8_t totalPages){
	if(idx>3) return 0xFF;
	uint16_t s=(idx<2) ? slotND : slotAR;
	uint8_t v=(idx & 1) ? (uint8_t)(s>>8) : (uint8_t)s;
	if(v==0 || v>totalPages) return 0xFF;
	return (uint8_t)(v-1);
}

void dnaPageSet(uint16_t *slotND, uint16_t *slotAR, uint8_t idx, uint8_t page){
	if(idx>3) return;
	uint16_t *s=(idx<2) ? slotND : slotAR;
	uint8_t v=(page==0xFF) ? 0 : (uint8_t)(page+1);
	if(*s==0xFFFF) *s=0; //blank flash
	if(idx & 1) *s=(uint16_t)((*s & 0x00FF) | ((uint16_t)v<<8));
	else        *s=(uint16_t)((*s & 0xFF00) | v);
}

//called every tick with the drive style. Returns the page to show once the style changed and stayed for 500 ms
uint8_t dnaPageDecide(uint32_t now, uint8_t mode384, uint16_t slotND, uint16_t slotAR, uint8_t totalPages){
	static uint8_t applied=0xFE, candidate=0xFE;
	static uint32_t candidateSince;
	uint8_t idx=dnaModeIndex(mode384);
	if(idx==0xFF) return 0xFF;
	if(idx==applied){ candidate=idx; return 0xFF; }
	if(idx!=candidate){ candidate=idx; candidateSince=now; return 0xFF; }
	if(now-candidateSince<500) return 0xFF;
	applied=idx;
	return dnaPageGet(slotND,slotAR,idx,totalPages);
}

uint16_t autoScrollPeriodMs(uint8_t mode){
	return mode==1 ? 3000 : (mode==2 ? 5000 : (mode==3 ? 10000 : 0));
}

//1: show the next page now. The timer restarts on every page change, whoever made it
uint8_t autoScrollDue(uint32_t now, uint8_t mode, uint8_t allowed, uint8_t currentPage){
	static uint32_t since;
	static uint8_t lastPage=0xFF, wasAllowed;
	uint16_t period=autoScrollPeriodMs(mode);
	if(!period || !allowed){ wasAllowed=0; return 0; }
	if(!wasAllowed || currentPage!=lastPage){ wasAllowed=1; lastPage=currentPage; since=now; return 0; }
	if(now-since<period) return 0;
	since=now;
	return 1;
}
