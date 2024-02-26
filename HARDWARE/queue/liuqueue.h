#ifndef __LIU_QUEUE_H
#define __LIU_QUEUE_H

#include "protocol.h"

uint8_t pop_event(msg_event_t *event);
uint8_t push_event(msg_event_t event);

uint8_t pop_ec20_event(msg_event_t *event);
uint8_t push_ec20_event(msg_event_t event);
void clear_ec20_event(void);

uint8_t pop_485_event(msg_event_t *event);
uint8_t push_485_event(msg_event_t event);
void clear_485_event(void);
#endif
