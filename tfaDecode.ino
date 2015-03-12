/*
  Receiver for temperature probes for wireless thermometers, tested with "TFA" brand.
  Reception with 433,92 MHz AM ASK module.

  Basic timing
  ------------
  Signal train is preceded by about 32 ms low (i.e. radio silence)
  Long high signal measured as about 1250 to 1650 µs stands for a logic 0
  Short high signal measured as about 450 to 750 µs stands for a logic 1
  Between high signals the low part is about 970 to 1050 µs

  Protocol
  --------
  Each sensor transmits the measured temperature every minute. It transmits 44 data bits, waits about 32 ms and sends
  the same 44 bits again. Numbering the bits from 0 to 43 they contain:
  0 -18: Sensor ID, chosen at random on insertion of batteries
     19: Parity of bits 20-31
  20-23: A
  24-27: B
  28-31: C
  32-35: A (repeated)
  36-39: B (repeated)
  40-43: Unknown (checksum?)

  The temperature in centigrades is (A*10 + B + C/10) - 50, or (AB,C) - 50 degrees.

  Code
  ----
  My code uses the duration of the high signal pulses to find relevant data, and I have analyzed a few thousand
  samples in order to find good limits for the short and long pulses. With the values chosen I will receive most
  true transmissions and discard a majority of incorrect data. I have also implemented a number of sanity checks on
  received data, checking that the unit ID's are my sensors' ID's, that the duplicated data is the same in both
  places, and that the temperature readings are within bounds. Any reading that seems faulty is just ignored.


  Johan Adler, started in October 2009

  Without Thomas Pfeifer's summary of the protocol basics I would not have gotten started on this project,
  but it would also have been difficult to do without using my logic analyzer from Saleae to get the basic timing.

  http://www.elektronikforumet.com/forum/viewtopic.php?f=3&t=22380
  http://thomaspfeifer.net/funk_temperatur_sensor_protokoll.htm
*/

void decodeTfa(rxDataStruct currentPulse) {
  #define tfaShort1Low 450
  #define tfaShort1High 750
  #define tfaLong0Low 1250
  #define tfaLong0High 1650
  #define tfaLowLow 750
  #define tfaLowHigh 1250

  static uint16_t tfaPulseLength[90];
  static uint8_t tfaPulseLevel[90], tfaPulseCount, tfaSignalCount;
  static char tfaSignal[50];
  uint8_t tfaParity = 0, tfaA = 0, tfaB = 0, tfaC = 0, i;
  uint32_t tfaId = 0;
  int16_t tfaTemp = 0;

  if(currentPulse.lvl == 1) {
    if(pulseInt(currentPulse.dur, tfaShort1Low, tfaShort1High)) {
      tfaSignal[tfaSignalCount++] = 1;
    } else
    if(pulseInt(currentPulse.dur, tfaLong0Low, tfaLong0High)) {
      tfaSignal[tfaSignalCount++] = 0;
    } else goto tfaTryAgain;
  }
  if(currentPulse.lvl == 0 && !pulseInt(currentPulse.dur, tfaLowLow, tfaLowHigh)) {
    goto tfaTryAgain;
  }
  tfaPulseLength[tfaPulseCount] = currentPulse.dur;
  tfaPulseLevel[tfaPulseCount++] = currentPulse.lvl;

  if(tfaSignalCount == 44) {
    for(i=19; i<32; i++) {
      tfaParity ^= tfaSignal[i];
    }
    if(tfaParity != 0) goto tfaTryAgain;

    for(i=0; i<4; i++)
      if(tfaSignal[i+20] != tfaSignal[i+32] || tfaSignal[i+24] != tfaSignal[i+36])
        goto tfaTryAgain;

    for(i=0; i<19; i++) tfaId += tfaSignal[i] << (18 - i);

    for(i=0; i<4; i++) {
      tfaA += tfaSignal[i + 20] << (3 - i);
      tfaB += tfaSignal[i + 24] << (3 - i);
      tfaC += tfaSignal[i + 28] << (3 - i);
    }
//    if(tfaA > 12 || tfaB > 9 || tfaC > 9) goto tfaTryAgain;

    tfaTemp = (tfaA * 100 + tfaB * 10 + tfaC) - 500;	// Store temperature * 10 as signed int

    output.print("T-t;");
    hexPrint(tfaId, 5);
    output.print(";");
    if(tfaTemp < 0) {
      output.print("-");
      tfaTemp = abs(tfaTemp);
    }
    output.print(tfaTemp / 10);
    output.print(".");
    output.print(tfaTemp % 10);
    output.print(";");
    for(i=0; i<tfaSignalCount; i++) {
      tfaSignal[i] += '0';
    }
    tfaSignal[tfaSignalCount] = 0;
    output.print(tfaSignal);
//    output.print(";");
//    output.print(tfaPulseCount,DEC);
//    for(i=0; i<tfaPulseCount; i++) {
//      output.print(";");
//      output.print(tfaPulseLength[i],DEC);
//      output.print(";");
//      output.print(tfaPulseRssi[i],DEC);
//      output.print(";");
//      output.print(tfaPulseLevel[i],DEC);
//    }
    output.println();
  } else {
    return;  // Expected number of bits not reached, but still trying
  }
  tfaTryAgain:  // Start over
  tfaPulseCount = tfaSignalCount = 0;
  return;
}

