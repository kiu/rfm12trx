#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef cbi
 #define cbi(sfr, bit)     (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
 #define sbi(sfr, bit)     (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#endif
