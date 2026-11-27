// host test for giulia h3, warnings, chime, page by drive style, auto scroll. Built from Core/Src/warnings.c as it is.
#include <stdio.h>
#include <string.h>
#include "warnings.h"
static int fails=0, checks=0;
#define CK(c,name) do{ int ok=(c); checks++; if(!ok) fails++; printf("%s %s\n",ok?"ok  ":"FAIL",name); }while(0)
static uint32_t now=1000;
static int chimes, shown, shownCold, shownLow; static char last[19];
// runs ms of time in 100 ms steps, like stage3Periodic, and draws the screen every 500 ms
static void run(uint32_t ms, uint32_t rpm, uint16_t ticks, uint8_t t, uint8_t p, uint32_t age){
	for(uint32_t k=0;k<ms;k+=100){
		now+=100;
		oilWarnEvaluate(now,rpm,ticks,t,p,age);
		if(warnChimePending(now)) chimes++;
		if(now%500==0){
			uint8_t b[18];
			if(warnOverlay(now,b)){ shown++; memcpy(last,b,18); last[18]=0; if(!memcmp(b,"OIL COLD",8)) shownCold++; if(!memcmp(b,"OIL PRES",8)) shownLow++; }
		}
	}
}
static void zero(void){ chimes=shown=shownCold=shownLow=0; last[0]=0; }
int main(void){
	warnReset();
	zero(); run(60000,2000,500,130,22,10); CK(shown==0 && chimes==0,"warm cruise 2000 rpm, oil 90 C 2.2 bar: silent for 60 s");
	zero(); run(60000,750,500,130,22,10);  CK(shown==0 && chimes==0,"warm idle 750 rpm 2.2 bar: silent");
	zero(); run(5000,5500,500,130,45,10);  CK(shown==0 && chimes==0,"warm 5500 rpm 4.5 bar: silent");
	zero(); run(200,4000,500,60,22,10); run(5000,2000,500,60,22,10); CK(shown==0 && chimes==0,"cold oil, 200 ms above 3500 rpm: ignored");
	zero(); run(1000,4000,500,60,45,10); CK(shownCold>=1 && chimes==1 && !strcmp(last,"OIL COLD: EASY RPM"),"cold oil 20 C, 4000 rpm for 1 s: text and one chime");
	zero(); run(1500,2000,500,60,22,10); int a=shown; run(3000,2000,500,60,22,10); zero(); run(6000,2000,500,60,22,10); CK(shown==0,"cold warning ends 3 s after the raise once rpm is back under 3000");
	(void)a;
	zero(); run(1000,4000,500,60,45,10); CK(shownCold>=1 && chimes==0,"second cold kickdown 12 s later: text, no chime (60 s rearm)");
	run(4000,2000,500,60,22,10); run(60000,2000,500,60,22,10);
	zero(); run(1000,4000,500,60,45,10); CK(chimes==1,"cold kickdown after more than 60 s: chime again");
	run(5000,2000,500,60,22,10);
	zero(); run(1000,3400,500,60,22,10); CK(shown==0,"cold oil at 3400 rpm: below the 3500 threshold");
	zero(); run(1000,4000,500,100,45,10); CK(shown==0,"oil exactly 60 C at 4000 rpm: no warning");
	zero(); run(1000,4000,500,99,45,10); CK(shownCold>=1,"oil 59 C at 4000 rpm: warning"); run(5000,2000,500,130,22,10);
	// engine start: no pressure yet
	zero(); run(3000,900,0,60,0,10); run(2000,900,300,60,5,10); CK(shown==0 && chimes==0,"first 5 s after the start with 0 bar: silent");
	zero(); run(10000,800,500,130,22,10); CK(shown==0,"then 2.2 bar: silent");
	zero(); run(1900,800,500,130,12,10); CK(shown==0 && chimes==0,"1.2 bar for 1.9 s: not yet");
	zero(); run(10000,800,500,130,12,10); CK(shownLow>=8 && shownLow<=10 && chimes==1 && !strcmp(last,"OIL PRESSURE LOW  "),"1.2 bar for 2 s and more: text about half of the time, one chime");
	printf("     overlay frames in 10 s: %d of 20\n",shownLow);
	zero(); run(25000,800,500,130,12,10); CK(chimes==1,"still low 30 s after the first chime: second chime");
	zero(); run(30000,800,500,130,12,10); CK(chimes==1,"60 s: third chime");
	zero(); run(120000,800,500,130,12,10); CK(chimes==0 && shownLow>50,"later: text stays, no more chimes");
	zero(); run(3000,800,500,130,16,10); CK(shownLow>=1,"back to 1.6 bar: still on (clears at 1.8)");
	run(500,800,500,130,18,10); zero(); run(5000,800,500,130,18,10); CK(shown==0,"1.8 bar: cleared");
	zero(); run(10000,800,500,130,12,3000); CK(shown==0 && chimes==0,"0x4B2 older than 2 s: no evaluation");
	zero(); run(10000,4000,500,0xFF,0xFF,10); CK(shown==0 && chimes==0,"raw 0xFF for both: ignored");
	zero(); run(10000,300,0,60,0,10); CK(shown==0 && chimes==0,"engine off, 0 bar: silent");
	zero(); run(10000,500,500,130,5,10); CK(shown==0,"below 600 rpm: no pressure warning");
	// low pressure with the engine cold and revving: critical wins the window
	run(3500,4000,500,60,12,10); zero(); run(6000,4000,500,60,12,10); CK(shownLow>=4 && shownCold==0,"cold and low pressure both active: only the critical one is drawn");
	run(3000,800,500,130,22,10); run(70000,800,500,130,22,10);
	// interface for stage 4: two plain warnings take turns, 1 s every 5 s
	{ warnReset(); now=1000000; int n1=0,n5=0; uint8_t b[18];
	  for(int k=0;k<400;k++){ now+=100; warnSet(1,1,0,"COOLANT HOT 108\xB0""C",now); warnSet(5,1,0,"IC AIR HOT 65\xB0""C",now);
	    if(now%500==0 && warnOverlay(now,b)){ if(b[0]=='C') n1++; else n5++; } }
	  CK(n1>=7 && n5>=7 && n1+n5<=18,"two plain warnings alternate, 1 s every 5 s"); printf("     frames in 40 s: %d and %d of 80\n",n1,n5);
	  warnSet(1,0,0,"",now); warnSet(5,0,0,"",now); CK(!warnOverlay(now,b),"cleared: overlay off at once");
	  warnSet(9,1,0,"x",now); CK(!warnOverlay(now,b),"slot out of range ignored");
	  warnSet(2,1,0,"A TEXT LONGER THAN 18 CHARS",now); now+=5000; CK(warnOverlay(now,b) && !memcmp(b,"A TEXT LONGER THAN",18),"long text cut to 18");
	  warnSet(2,1,0,"SHORT",now); now+=100; CK(warnOverlay(now,b) && !memcmp(b,"SHORT             ",18),"short text padded with spaces"); warnReset(); }
	// drive style pages
	{ uint16_t nd=0, ar=0xFFFF; 
	  CK(dnaPageGet(0,0,0,54)==0xFF && dnaPageGet(0xFFFF,0xFFFF,1,54)==0xFF,"slots 0 and blank flash: nothing bound");
	  dnaPageSet(&nd,&ar,0,49); dnaPageSet(&nd,&ar,1,0); dnaPageSet(&nd,&ar,3,53);
	  CK(nd==0x0132 && ar==0x3600,"N=49, D=0, R=53 packed as page+1"); printf("     slot35=0x%04X slot36=0x%04X\n",nd,ar);
	  CK(dnaPageGet(nd,ar,0,54)==49 && dnaPageGet(nd,ar,1,54)==0 && dnaPageGet(nd,ar,2,54)==0xFF && dnaPageGet(nd,ar,3,54)==53,"read back, A not bound");
	  CK(dnaPageGet(nd,ar,3,47)==0xFF,"page beyond the table (diesel or older build): not bound");
	  dnaPageSet(&nd,&ar,0,0xFF); CK(dnaPageGet(nd,ar,0,54)==0xFF && dnaPageGet(nd,ar,1,54)==0,"unbind N keeps D");
	  dnaPageSet(&nd,&ar,0,49);
	  CK(dnaModeIndex(0x00)==0 && dnaModeIndex(0x08)==1 && dnaModeIndex(0x10)==2 && dnaModeIndex(0x30)==3 && dnaModeIndex(0x04)==0xFF,"0x384 codes");
	  uint32_t t=5000; uint8_t r=0xFF; int jumps=0;
	  for(int k=0;k<10;k++){ t+=100; r=dnaPageDecide(t,0x00,nd,ar,54); if(r!=0xFF){ jumps++; } }
	  CK(jumps==1,"boot in N: bound page applied once, after 500 ms");
	  jumps=0; for(int k=0;k<3;k++){ t+=100; if(dnaPageDecide(t,0x08,nd,ar,54)!=0xFF) jumps++; } for(int k=0;k<10;k++){ t+=100; if(dnaPageDecide(t,0x00,nd,ar,54)!=0xFF) jumps++; }
	  CK(jumps==0,"D for 300 ms then back to N: no jump");
	  jumps=0; r=0xFF; for(int k=0;k<10;k++){ t+=100; uint8_t x=dnaPageDecide(t,0x08,nd,ar,54); if(x!=0xFF){ jumps++; r=x; } }
	  CK(jumps==1 && r==0,"N to D: jump to page 0");
	  jumps=0; for(int k=0;k<10;k++){ t+=100; if(dnaPageDecide(t,0x10,nd,ar,54)!=0xFF) jumps++; }
	  CK(jumps==0,"D to A, A not bound: page stays");
	  jumps=0; for(int k=0;k<10;k++){ t+=100; if(dnaPageDecide(t,0x7C,nd,ar,54)!=0xFF) jumps++; }
	  CK(jumps==0,"unknown code: ignored"); }
	// auto scroll
	{ uint32_t t=0; int due=0; uint8_t page=3;
	  CK(autoScrollPeriodMs(0)==0 && autoScrollPeriodMs(1)==3000 && autoScrollPeriodMs(2)==5000 && autoScrollPeriodMs(3)==10000 && autoScrollPeriodMs(9)==0,"periods");
	  for(int k=0;k<300;k++){ t+=100; if(autoScrollDue(t,2,1,page)){ due++; page++; } }
	  CK(due==5,"5 s mode: 5 page changes in 30 s"); printf("     changes: %d\n",due);
	  due=0; for(int k=0;k<300;k++){ t+=100; if(k%40==0) page++; if(autoScrollDue(t,2,1,page)) due++; }
	  CK(due==0,"driver changes the page every 4 s: auto scroll waits");
	  due=0; for(int k=0;k<300;k++){ t+=100; if(autoScrollDue(t,2,0,page)) due++; }
	  CK(due==0,"not allowed (menu, hold mode, engine off): no scroll");
	  due=0; for(int k=0;k<49;k++){ t+=100; if(autoScrollDue(t,2,1,page)) due++; }
	  CK(due==0,"allowed again: a full period first");
	  due=0; for(int k=0;k<300;k++){ t+=100; if(autoScrollDue(t,0,1,page)) due++; }
	  CK(due==0,"mode OFF"); }
	// driver strings
	{ const char *s[]={"OIL COLD: EASY RPM","OIL PRESSURE LOW","DNA N: not bound","Auto Scroll OFF","Auto Scroll 10s","O  Warnings +Chime"}; int fit=1;
	  for(int i=0;i<6;i++){ printf("     %2d [%s]\n",(int)strlen(s[i]),s[i]); if(strlen(s[i])>18) fit=0; } CK(fit,"driver strings fit 18 chars"); }
	printf("checks=%d fails=%d\n",checks,fails); return fails;
}
