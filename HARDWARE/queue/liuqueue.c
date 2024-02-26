#include "liuqueue.h"

#define MAX_QUEUE_SIZE 64
static uint8_t popindex = 0;
static uint8_t pushindex = 0;
static uint8_t size = 0;
msg_event_t msgEvents[MAX_QUEUE_SIZE];


static uint8_t ec20_popindex = 0;
static uint8_t ec20_pushindex = 0;
static uint8_t ec20_size = 0;
msg_event_t EC20_events[MAX_QUEUE_SIZE];

static uint8_t rs485_popindex = 0;
static uint8_t rs485_pushindex = 0;
static uint8_t rs485_size = 0;
msg_event_t rs485_events[MAX_QUEUE_SIZE];

/*
*函数描述:弹出消息队列中的事件
*返回值:0-->失败，消息队列中没有数据 1-->成功 事件赋值在传过来的事件指针里
*/
uint8_t pop_event(msg_event_t *event)
{
	if(size == 0)
	{
		return 0;
	}
	
	*event = msgEvents[popindex];
	popindex++;
	if(popindex >= MAX_QUEUE_SIZE)
		popindex = 0;
	size = size - 1;
	return 1;
}

/*
*函数描述:添加事件到消息队列中
*返回:0-->失败，消息队列已满  1-->成功
*/
uint8_t push_event(msg_event_t event)
{
	if(size >= MAX_QUEUE_SIZE)
		return 0;
	
	msgEvents[pushindex] = event;
	pushindex++;
	if(pushindex >= MAX_QUEUE_SIZE)
		pushindex = 0;
	size = size + 1;
	return 1;
}

/*
*函数描述:弹出消息队列中的事件
*返回值:0-->失败，消息队列中没有数据 1-->成功 事件赋值在传过来的事件指针里
*/
uint8_t pop_ec20_event(msg_event_t *event)
{
	if(ec20_size <= 0)
	{
		return 0;
	}
	
	*event = EC20_events[ec20_popindex];
	ec20_popindex++;
	
	if(ec20_popindex >= MAX_QUEUE_SIZE)
		ec20_popindex = 0;
	
	ec20_size = ec20_size - 1;
	
	return 1;
}

/*
*函数描述:添加事件到消息队列中
*返回:0-->失败，消息队列已满  1-->成功
*/
uint8_t push_ec20_event(msg_event_t event)
{
	if(ec20_size >= MAX_QUEUE_SIZE)
		return 0;
	
	EC20_events[ec20_pushindex] = event;
	ec20_pushindex++;
	
	if(ec20_pushindex >= MAX_QUEUE_SIZE)
		ec20_pushindex = 0;
	
	ec20_size = ec20_size + 1;
	
	return 1;
}

void clear_ec20_event()
{
	ec20_pushindex = 0;
	ec20_popindex = 0;
	ec20_size = 0;
}

uint8_t pop_485_event(msg_event_t *event)
{
	if(rs485_size == 0)
	{
		return 0;
	}
	
	*event = rs485_events[rs485_popindex];
	rs485_popindex++;
	if(rs485_popindex >= MAX_QUEUE_SIZE)
		rs485_popindex = 0;
	rs485_size = rs485_size - 1;
	return 1;
}
uint8_t push_485_event(msg_event_t event)
{
	if(rs485_size >= MAX_QUEUE_SIZE)
		return 0;
	
	rs485_events[rs485_pushindex] = event;
	rs485_pushindex++;
	
	if(rs485_pushindex >= MAX_QUEUE_SIZE)
		rs485_pushindex = 0;
	
	rs485_size = rs485_size + 1;
	
	return 1;
}

void clear_485_event(void)
{
	rs485_pushindex = 0;
	rs485_popindex = 0;
	rs485_size = 0;
}

