//see page_order.h
#include "page_order.h"

uint8_t pageValueCount(const uds_params_couple_element *p){
	if(p->values==3) return 3;
	return (p->udsParamId[0]!=p->udsParamId[1]) ? 2 : 1;
}

uint8_t pageOrderBuild(const uds_params_couple_element *table, uint8_t total, uint8_t *order){
	uint8_t n=0, combined=0;
	for(uint8_t want=PAGE_VALUES_MAX; want>=1; want--){
		for(uint8_t i=0;i<total;i++) if(pageValueCount(&table[i])==want) order[n++]=i;
		if(want==2) combined=n;
	}
	return combined;
}

uint8_t pageOrderStep(const uint8_t *order, uint8_t total, const uint8_t *shown, uint8_t cur, int8_t delta){
	if(total==0) return 0;
	int16_t pos=0;
	for(uint8_t i=0;i<total;i++) if(order[i]==cur){ pos=i; break; }
	pos+=delta;
	if(pos>=total) pos=0;
	if(pos<0) pos=total-1;
	int8_t dir=(delta<0) ? -1 : 1;
	for(uint8_t k=0;k<total;k++){
		if(shown[order[pos]]) return order[pos];
		pos+=dir;
		if(pos>=total) pos=0;
		if(pos<0) pos=total-1;
	}
	return (cur<total) ? cur : order[0]; //all hidden
}

uint8_t pageMenuEntry(const uint8_t *order, uint8_t total, uint8_t combined, uint8_t pos){
	if(pos==combined || pos>total) return PAGE_MENU_SAVE;
	return order[pos<combined ? pos : (uint8_t)(pos-1)];
}

uint8_t pageMenuStep(uint8_t total, uint8_t pos, int8_t delta){
	int16_t p=(int16_t)pos+delta;
	if(p>total+1) return 0;
	if(p<0) return (uint8_t)(total+1);
	return (uint8_t)p;
}

uint8_t pageNextElement(uint8_t mask, uint8_t count, uint8_t cur){
	for(uint8_t k=1;k<=count;k++){
		uint8_t i=(uint8_t)((cur+k)%count);
		if(mask & (1u<<i)) return i;
	}
	return 0xFF;
}
