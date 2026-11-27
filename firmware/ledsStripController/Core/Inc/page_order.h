//combined pages: how many values a page has, the order pages are shown in, and the Params Setup list.
//Pure logic: no HAL, no globals. Also built by the host test.
#ifndef INC_PAGE_ORDER_H_
#define INC_PAGE_ORDER_H_
	#include "uds_parameters.h"

	#define PAGE_VALUES_MAX 3
	#define PAGE_MENU_SAVE 0xFF

	uint8_t pageValueCount(const uds_params_couple_element *p);		//1, 2 or 3
	//order gets the page numbers: three values first, then two, then one; table order inside each group.
	//Returns how many combined pages (two or three values) come first. Page numbers themselves do not change.
	uint8_t pageOrderBuild(const uds_params_couple_element *table, uint8_t total, uint8_t *order);
	//moves delta places along the order from page cur, wraps at both ends, then skips hidden pages
	uint8_t pageOrderStep(const uint8_t *order, uint8_t total, const uint8_t *shown, uint8_t cur, int8_t delta);
	//Params Setup list: combined pages, SAVE&EXIT, single pages, SAVE&EXIT. total+2 positions.
	uint8_t pageMenuEntry(const uint8_t *order, uint8_t total, uint8_t combined, uint8_t pos);	//page number or PAGE_MENU_SAVE
	uint8_t pageMenuStep(uint8_t total, uint8_t pos, int8_t delta);
	//next element after cur (cyclic) whose bit is set in mask, 0xFF if the mask is empty
	uint8_t pageNextElement(uint8_t mask, uint8_t count, uint8_t cur);
#endif
