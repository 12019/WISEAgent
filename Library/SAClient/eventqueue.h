#ifndef _EVTQUEUE_H_
#define _EVTQUEUE_H_
#include <stdbool.h>
#include "susiaccess_def.h"

typedef void (*EVENT_UPDATECONNECTSTATE)(susiaccess_agent_conf_body_t const * conf, susiaccess_agent_profile_body_t const * profile, int status);

bool evtqueue_init(const unsigned int slots, EVENT_UPDATECONNECTSTATE func);

void evtqueue_uninit();

bool evtqueue_push(susiaccess_agent_conf_body_t const * conf, susiaccess_agent_profile_body_t const * profile, int status);

void evtqueue_clear();

#endif
