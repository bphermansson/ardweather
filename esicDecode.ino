/*
    Decode signal from Esic brand wireless thermometer/hygrometer.
    My sensor/transmitter units are bought at Clas Ohlson,
    product number 36-1794 (for main unit with one sensor)
    (http://www.clasohlson.se/link/m3/Product,Product.aspx?artnr=36-1794).
    Extra sensors are available as 36-1797.
    These products are manufactured by W.H. Mandolyn International Ltd.
    The sensor unit has their product ID WT450H, receiver unit WS2015H.
    All units marked as Esic brand.

    The signal is FM encoded with clock cycle around 2000 µs
    No level shift within the clock cycle translates to a logic 0
    One level shift within the clock cycle translates to a logic 1
    Each clock cycle begins with a level shift
    My timing constants defined below are those observed by my program

    +---+   +---+   +-------+       +  high
    |   |   |   |   |       |       |
    |   |   |   |   |       |       |
    +   +---+   +---+       +-------+  low

    ^       ^       ^       ^       ^  clock cycle
    |   1   |   1   |   0   |   0   |  translates as

    Each transmission is 36 bits long (i.e. 72 ms)

    Data is transmitted as pure binary values, NOT BCD-coded.

    -----------------------------------------------------------------------

    Update 100821: New information from Stefan of the NetHome project
    (http://wiki.nethome.nu/doku.php/start) regarding coding of transmission,
    shared at http://elektronikforumet.com/forum/viewtopic.php?p=601250#p601250

    Example transmission (House 1, Channel 1, RH 59 %, Temperature 23.5 °C)
    110000010011001110110100100110011000

    bit  0,  4 bits: (C1) Constant 1100, preamble?
    bit  4,  4 bits: (hc) House code (transmitters only seem to use values 1 - 15)
    bit  8,  2 bits: (cc) Channel code - 1
    bit 10,  2 bits: (C2) Constant 11, unknown, unused part of CC?
    bit 12,  1 bit : (lb) Low battery (active high)
    bit 13,  8 bits: (rh) RH * 2
    bit 21, 11 bits: (t)  (Temp + 50) * 16
    bit 32,  2 bits: (sq) Sequence number in transmission burst - 1
    bit 34,  1 bit : (p1) Parity1 (xor of all bits should result in 1?)
    bit 35,  1 bit : (p2) Parity2 (xor of all bits should result in 0)

    Decoding actual values:
    House code	    = hc
    Channel code    = cc + 1
    Low battery	    = (boolean)(lb == 1)
    Rel humidity    = rh / 2
    Temperature	    = t / 16 - 50
    Sequence num    = sq + 1 (if you prefer 1-based sequence)
    Parity	    = ...

    Transmission example, hc 1, cc 3, batt good, rh 67 %, t 22.2 °C
    1100 0001 10 11 0 10000110 10010000011 00 0 1	seq num 1
    1100 0001 10 11 0 10000110 10010000011 01 0 0	seq num 2
    1100 0001 10 11 0 10000110 10010000011 10 1 1	seq num 3


    -----------------------------------------------------------------------

    Example transmission (House 1, Channel 1, RH 59 %, Temperature 23.5 °C)
    110000010011001110110100100110011000

    b00 - b03  (4 bits): (c1) Constant, 1100, probably preamble
    b04 - b07  (4 bits): (hc) House code (here: 0001 = HC 1)
    b08 - b09  (2 bits): (cc) Channel code - 1 (here 00 = CC 1)
    b10 - b12  (3 bits): (c2) Constant, 110
    b13 - b19  (7 bits): (rh) Relative humidity (here 0111011 = 59 %)
    b20 - b34 (15 bits): (T)  Temperature (see below)
    b35 - b35  (1 bit) : Parity (xor of all bits should give 0)

    The temperature is transmitted as (temp + 50.0) * 128,
    which equals (temp * 128) + 6400. Adding 50.0 °C makes
    all values positive, an unsigned 15 bit integer where the
    first 8 bits correspond to the whole part of the temperature
    (here 01001001, decimal 73, substract 50 = 23).
    Remaining 7 bits correspond to the fractional part.

    To avoid floating point calculations I store the raw temperature value
    as a signed integer in the variable esicTemp, then transform it to
    actual temperature * 10 using "esicTemp = (esicTemp - 6400) * 10 / 128",
    where 6400 is the added 50 times 128.
    When reporting the temperature I simply print "esicTemp / 10" (integer division,
    no fraction), followed by a decimal point and "esicTemp % 10" (remainder, which
    equals first fractional decimal digit).

    Summary of bit fields:
    1100 0001 00 110 0111011 010010011001100 0
     c1   hc  cc  c2    rh          t        p

    c1, c2 = constant field 1 and 2
    hc, cc = house code and channel code
    rh, t  = relative humidity, temperature
    p      = parity bit

    Main decoding was done by Øyvind Kaurstad (http://personal.dynator.no/),
    who reported about his work back in 2006 at
    http://www.varmepumpsforum.com/vpforum/index.php?topic=3145.msg101023#msg101023
    (Swedish forum, Øyvind writes in Norwegian).
    On my request he let me share his findings (a spreadsheet analyzing signals
    as captured by a digital oscilloscope), which made it quite easy for me to
    write the actual code that decodes the wireless signal I receive.
*/

void decodeEsic(rxDataStruct currentPulse) {
  #define esicShortLow 750
  #define esicShortHigh 1200
  #define esicLongLow 1800
  #define esicLongHigh 2100

  static uint16_t pulseLength[75];  // Store actual pulse lengths for debugging/analyzing/optimizing
  static uint8_t pulseLevel[75], pulseCount, signalCount, halfClockCycles;
  static char signal[38];
  const uint8_t esicConst1[4] = {1, 1, 0, 0};
  const uint8_t esicConst2[2] = {1, 1};
  uint8_t esicParityOdd = 1, esicParityEven = 0,
    esicHouse = 0, esicChannel = 1, esicLoBatt = 0, esicSeq = 1, i;
  //uint16_t esicRH = 0;
  int16_t esicRH = 0;
  int16_t esicTemp = 0, etdint, ehdint;
  

//uint8_t is the same as a byte.

  if((pulseCount > 0 && currentPulse.lvl == pulseLevel[pulseCount - 1]))
    goto esicTryAgain;  // We have started to decode a signal, but the logical level is the same as the last pulse

  if(pulseInt(currentPulse.dur, esicShortLow, esicShortHigh)) {
    halfClockCycles++;
    if(halfClockCycles % 2 == 0) {
      signal[signalCount++] = 1;
    }
  } else
  if(pulseInt(currentPulse.dur, esicLongLow, esicLongHigh)) {
    halfClockCycles += 2;
    signal[signalCount++] = 0;
  } else goto esicTryAgain;

   i = signalCount - 1;  // Check for the two constant fields
   if(i < 4) {
     if(signal[i] != esicConst1[i])
       goto esicTryAgain;
   }
   
  pulseLength[pulseCount] = currentPulse.dur;
  pulseLevel[pulseCount++] = currentPulse.lvl;

  if(signalCount == 36) { // Expected number of signals/bits
//    for(i=0; i<4; i++) { if(signal[i] != esicConst1[i]) { goto esicTryAgain; } }    // Check for the two constant fields
//     for(i=0; i<2; i++) { if(signal[i + 10] != esicConst2[i]) { goto esicTryAgain; } }

//    for(i=0; i<signalCount; i++) {  // Parity check
//      esicParityOdd ^= signal[i];
//      esicParityEven ^= signal[i];
//    }
//    if(esicParityOdd != 0 || esicParityEven != 0)
//      goto esicTryAgain;

    for(i=0; i<4; i++) {
      esicHouse |= signal[i + 4] << (3 - i);
    }
    // Convert house value 
    itoa (esicHouse, houseChar, 10);
    
    for(i=0; i<2; i++) {
      esicChannel += signal[i + 8] << (1 - i);
    }
    // Convert channel value 
    itoa (esicChannel, channelChar, 10);

    esicLoBatt = signal[12];

    for(i=0; i<8; i++) {
      esicRH |= signal[i + 13] << (7 - i);
    }
    esicRH *= 5;

    for(i=0; i<11; i++) {
      esicTemp |= signal[i + 21] << (10 - i);
    }
    esicTemp = (esicTemp - 800) * 10 / 16;
//     esicTemp = (esicTemp - 6400) * 10 / 128;

    for(i=0; i<2; i++) {
      esicSeq += signal[i + 32] << (1 - i);
    }

#if defined(debugEsic)
    output.println();

    output.print("E-th;");
    output.print(esicHouse, DEC);
    output.print(";");
    output.print(esicChannel, DEC);
    output.print(";");
    if(esicTemp < 0) {
      output.print("-");
      esicTemp = abs(esicTemp);
    }
    output.print(esicTemp / 10);
    output.print(".");
    output.print(esicTemp % 10);
    output.print(";");
    
    output.print(esicRH/10);
    output.print(".");
    output.print(esicRH % 10);
    output.println(";");
#endif

    
    // Convert temp value 
    //itoa (esicTemp, charTemp[esicHouse][esicChannel], 10); 
    
    measurement[esicHouse][esicChannel].temp = esicTemp;
    measurement[esicHouse][esicChannel].humidity = esicRH;
    measurement[esicHouse][esicChannel].lobatt = esicLoBatt;
      
    intNewData=1;
 
  } else { // Expected number of signals/bits not reached, but still trying
#if defined(debugEsic)
    //output.print(".");
#endif
    return;
  }
  esicTryAgain:  // Start over
#if defined(debugEsic)
  if(signalCount > 1 && signalCount < 36) {
    output.print(signalCount, DEC);
    output.print(";");
  }
#endif
  pulseCount = signalCount = halfClockCycles = 0;
//  output.println();
  return;
}

