#define icpRxQSize 9
#define rxT1Pre (T1P_256)

rxDataStruct icp_rx_q[icpRxQSize];
uint8_t icp_rx_tail;
uint8_t icp_rx_head;


ISR(TIMER1_CAPT_vect) {	// Input Capture
  uint16_t icr;
  uint8_t tccr1b, sreg;

  icr = ICR1;	// Store counter timestamp
  tccr1b = TCCR1B;
  TCCR1B = tccr1b ^ _BV(ICES1);	// Reverse edge detect
  TIFR1 |= _BV(ICF1); // clear ICF flag

  icp_enq(icr, (tccr1b & _BV(ICES1))?0:1);

  sreg = SREG;
  cli();
  TCNT1 = 0;
  SREG = sreg;
  return;
}

__inline__ static void icp_enq(uint16_t dur, uint8_t lvl) {
  uint8_t t;

  t = icp_rx_tail;
  icp_rx_q[t].dur = dur << 4;
  icp_rx_q[t].lvl = lvl;
  if (++t >= icpRxQSize)
    t = 0;
  if (t != icp_rx_head)
    icp_rx_tail = t;
  return;
}

boolean icpAvail(void) {
  return(icp_rx_head != icp_rx_tail);
}

rxDataStruct icpRead(void) {
  rxDataStruct r;
  uint8_t h;

  h = icp_rx_head;
  if(h == icp_rx_tail) {
    r.dur = 0x0000;
    r.lvl = 0xff;
  } else {
    r = icp_rx_q[h];
    if (++h >= icpRxQSize)
      h = 0;
    icp_rx_head = h;
  }
  return(r);
}

void icp_init(void) {
  TCCR1A = 0;
  OCR1A = 0;
  TCCR1B = _BV(ICES1) | rxT1Pre | _BV(ICNC1);
  TIMSK1 = _BV(ICIE1);
}

