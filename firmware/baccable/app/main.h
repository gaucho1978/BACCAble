#ifndef BACCABLE_APP_MAIN_H
#define BACCABLE_APP_MAIN_H

#include "app/application_state.h"
#include "platform/system.h"

#include "vehicle/diagnostic_frames.h"
#include "vehicle/standard_frames.h"

#if defined(BACCABLE_C1)
    #include "app/powertrain.h"
#endif

#if defined(BACCABLE_BH)
    #include "features/body.h"
#endif

#endif /* BACCABLE_APP_MAIN_H */
