/* prescalers for timer 1

Given system clock CK of 16 MHz:

presc	f(T1)	µs	tick->ms	rollover (ms)	(s)
1	16MHz	0,0625	tick >> 4	4,096		0,004
8	2MHz	0,5	tick >> 1	32,768		0,033
64	256kHz	4	tick << 2	262,144		0,262
256	64kHz	16	tick << 4	1048,576	1,049
1024	16kHz	64	tick << 6	4194,304	4,194

1 tick = (presc/16) µs
		or in general form: 1 tick = (prescaler / system clock) seconds
*/


// prescalers for timer 1
#define T1P_1		(				_BV(CS10))
#define T1P_8		(		_BV(CS11))
#define T1P_64		(		_BV(CS11) |	_BV(CS10))
#define T1P_256		(_BV(CS12))
#define T1P_1024	(_BV(CS12) |			_BV(CS10))

// struct for storing relevant values about each registered pulse
struct rxDataStruct {
  uint16_t dur;  // pulse duration
  uint8_t lvl;   // logic level (really just needs one bit, will be set to 0 or 1, other values could be used for special messages)
};

// predefine function used for reading rx queue
rxDataStruct icpRead();

