#ifndef BACCABLE_FEATURES_BODY_H
#define BACCABLE_FEATURES_BODY_H

#include "app/application_state.h"

void body_init();
void body_process();
uint8_t mirror_positions_save();
uint16_t mirror_positions_read(uint8_t paramId);

#endif /* BACCABLE_FEATURES_BODY_H */
