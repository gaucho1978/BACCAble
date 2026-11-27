//giulia h4: background watchdogs (temperatures, new engine faults) and instant consumption.
//Pure logic: no HAL, no globals. Also built by the host test.
#ifndef INC_GIULIA_WATCH_H_
#define INC_GIULIA_WATCH_H_
	#include <stdint.h>

	//compile time switches: set to 0 while the fact is not verified on the car
	#ifndef GW_CAC_VERIFIED
	#define GW_CAC_VERIFIED 0		//1 = DID 1C16 confirmed as charge air cooler coolant temperature
	#endif

	#define GW_SLOT_MS			1000	//one background request per second at most
	#define GW_DTC_PERIOD_MS	10000	//19 01 every 10 s
	#define GW_GUARD_MS			120		//distance kept from the request of the page on screen
	#define GW_STALE_MS			15000	//a value older than this is not judged

	#define GW_COOLANT_HOT_ON	108
	#define GW_COOLANT_HOT_OFF	104
	//a condition has to hold this long (the page on screen can feed the same DID several times a second)
	#define GW_HOT_HOLD_MS		5000UL
	#define GW_COLD_HOLD_MS		10000UL
	#define GW_ICAIR_HOLD_MS	10000UL
	#define GW_RESUME_MS		180000UL	//an engine stop shorter than this (Start&Stop) keeps the state
	#define GW_COLD_RUN_MS		(20UL*60UL*1000UL)
	#define GW_COLD_FAST_MS		120000UL
	#define GW_COLD_SPEED		80
	#define GW_ICAIR_HOT_ON		65
	#define GW_ICAIR_HOT_OFF	58
	#define GW_ICAIR_SPEED		30
	#define GW_CAC_JUMP			30
	#define GW_CAC_LOW			(-35)
	#define GW_CAC_HIGH			150
	#define GW_DTC_MASK			0x0D	//testFailed | pending | confirmed
	#define GW_DTC_NRC_LIMIT	3

	#define GW_ITEM_CAC		0
	#define GW_ITEM_WATER	1
	#define GW_ITEM_ICAIR	2
	#define GW_ITEM_DTC		3
	#define GW_ITEM_NONE	0xFF

	#define GW_AL_COOLANT_HOT	0x01
	#define GW_AL_COOLANT_COLD	0x02
	#define GW_AL_ICAIR_HOT		0x04
	#define GW_AL_CAC_DROP		0x08
	#define GW_AL_NEW_FAULT		0x10

	//watch mode, flash slot 38: 0 = all (default), 1 = temperatures only, 2 = faults only, 3 = off
	#define GW_MODE_HAS_TEMP(m)	((m)==0 || (m)==1)
	#define GW_MODE_HAS_DTC(m)	((m)==0 || (m)==2)

	typedef struct{
		uint32_t startMs;		//engine start (gwReset)
		uint32_t lastPollMs;
		uint32_t lastDtcMs;
		uint32_t slowSinceMs;	//last moment the speed was below GW_COLD_SPEED
		uint32_t tWater, tIc, tCac;
		uint32_t hotSince, coldSince, icSince;
		int16_t  water, icAir, cac, cacMin;
		uint16_t dtcCount, dtcBase;
		uint8_t  slot;
		uint8_t  have;			//bit per GW_ITEM_x: a value arrived since the reset
		uint8_t  active;		//GW_AL_x currently true
		uint8_t  hotN, coldN, icN;
		uint8_t  cacDrops;
		uint8_t  dtcNrc;		//consecutive refusals
		uint8_t  dtcOff;		//1 = service refused, not asked again until the next engine start
		uint8_t  dtcMask;
	} gw_state;

	void    gwReset(gw_state *s, uint32_t now);
	uint8_t gwEngineStart(gw_state *s, uint32_t now, uint32_t stopMs, uint8_t hadRun);	//1: state reset, 0: resumed
	uint8_t gwNextItem(gw_state *s, uint32_t now, uint8_t mode);
	uint8_t gwBuildRequest(const gw_state *s, uint8_t item, uint8_t data[8]);
	uint8_t gwOnReply(gw_state *s, const uint8_t *rx, uint8_t dlc, uint32_t now, float speedKmh, uint8_t coldLimit);
	uint8_t gwAlarmText(const gw_state *s, uint8_t alarm, char out[19]);
	uint8_t gwColdLimit(uint8_t slot39);
	float   gwFuelPer100(float litersPerHour, float speedKmh);
#endif
