#ifndef INC_WARNINGS_H_
#define INC_WARNINGS_H_
//giulia h3: warning overlay, chime requests, page bound to the drive style, auto scroll.
//Pure logic: no HAL, no globals of the firmware. Built into the host test as it is.
#include <stdint.h>

#define WARN_SLOTS				8	//slot number = priority, 0 is the highest
#define WARN_TEXT_LEN			18	//DASHBOARD_MESSAGE_MAX_LENGTH
#define WARN_ID_OIL_PRESSURE	0	//stage 3
//slots 1..5: stage 4 (coolant hot, IC air hot, CAC sensor drop, new ECM fault, spare)
#define WARN_ID_OIL_COLD		6	//stage 3
//slot 7: stage 4 (coolant cold)

#define WARN_F_CRITICAL			0x01	//1 s every 2 s instead of 1 s every 5 s, chime repeated, shown with the menu hidden
#define WARN_F_CHIME			0x02	//asks one chime when raised

#define WARN_SHOW_MS			1000	//overlay length, same as >>4WD DISABLED<<
#define WARN_PERIOD_MS			5000	//overlay period, same as >>4WD DISABLED<< on the parameters page
#define WARN_PERIOD_CRIT_MS		2000
#define WARN_CHIME_GAP_MS		3000	//two chimes are never closer than this
#define WARN_CHIME_REARM_MS		60000	//the same warning raised again chimes only after this
#define WARN_CHIME_REPEAT_MS	30000	//critical warning still active: chime again
#define WARN_CHIME_REPEAT_MAX	3		//chimes for one raise of a critical warning

//thresholds, raw units of 0x4B2 (see docs/baccable-stage3-plan.md, all marked as guesses there)
#define WARN_OILT_COLD_RAW		100		//60 C (raw-40)
#define WARN_OILT_RPM_ON		3500
#define WARN_OILT_RPM_OFF		3000
#define WARN_OILT_ON_MS			300
#define WARN_OILT_MIN_MS		3000	//once raised it stays at least this long
#define WARN_OILP_LOW_RAW		15		//1.5 bar (raw*0.1)
#define WARN_OILP_CLEAR_RAW		18		//1.8 bar
#define WARN_OILP_RPM			600
#define WARN_OILP_ON_MS			2000
#define WARN_OIL_STALE_MS		2000	//0x4B2 older than this: no evaluation
#define WARN_ENGINE_ON_TICKS	500		//engineOnSinceMoreThan5seconds, 5 s

void    warnReset(void);
void    warnSet(uint8_t id, uint8_t active, uint8_t flags, const char *text, uint32_t now);
uint8_t warnOverlay(uint32_t now, uint8_t *dst18);	//1: dst18 holds the warning line and the page shall not be drawn
uint8_t warnCriticalActive(void);
uint8_t warnChimePending(uint32_t now);				//1: send one chime request now
void    oilWarnEvaluate(uint32_t now, uint32_t rpm, uint16_t engineOnTicks, uint8_t oilTraw, uint8_t oilPraw, uint32_t oilFrameAge);

uint8_t dnaModeIndex(uint8_t mode384);				//0 N, 1 D, 2 A, 3 R, 0xFF unknown
uint8_t dnaPageGet(uint16_t slotND, uint16_t slotAR, uint8_t idx, uint8_t totalPages);	//bound page, 0xFF none
void    dnaPageSet(uint16_t *slotND, uint16_t *slotAR, uint8_t idx, uint8_t page);		//page 0xFF clears
uint8_t dnaPageDecide(uint32_t now, uint8_t mode384, uint16_t slotND, uint16_t slotAR, uint8_t totalPages);	//page to jump to, 0xFF none
uint16_t autoScrollPeriodMs(uint8_t mode);			//0 off, 1 3 s, 2 5 s, 3 10 s
uint8_t autoScrollDue(uint32_t now, uint8_t mode, uint8_t allowed, uint8_t currentPage);
#endif
