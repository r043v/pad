#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#include "Arduino.h"

//#define pad_noSelectPin

namespace pad {

#pragma pack(push, 1)
union keys {
	u16 raw;
	struct {
	  union {
	    u8 byte1;
	    struct {
		  u8 up : 1;
		  u8 down : 1;
		  u8 left : 1;
		  u8 right : 1;
		  u8 : 2;
		  u8 onBoard1 : 1; /* on board adapter buttons */
		  u8 onBoard2 : 1;
	    };
	    struct { /* genesis */
		  u8 : 4;
		  u8 b : 1;
		  u8 c : 1;
		  u8 : 2;
	    };
	    struct { /* master system */
		  u8 : 4;
		  u8 one : 1;
		  u8 two : 1;
		  u8 : 2;
	    };
	  };
	  union {
	    u8 byte2;
	    struct { /* genesis */
		  u8 mode : 1;
		  u8 x : 1;
		  u8 y : 1;
		  u8 z : 1;
  		  u8 a : 1;
		  u8 start : 1;
		  u8 : 2;
	    };
	  };
	};
} keys;
#pragma pack(pop)

  u8 type, latch, clock, data;
  
#ifndef pad_noSelectPin
  u8 select;
#endif

  u8 detect(void);

  void config(u8 latch, u8 clock, u8 data
#ifndef pad_noSelectPin
  , u8 select
#endif
  ){
      pad::latch = latch;
      pinMode(latch,OUTPUT);
      pad::clock = clock;
      pinMode(clock,OUTPUT);
      pad::data = data;
      pinMode(data,INPUT);
#ifndef pad_noSelectPin
      pad::select = select;
      pinMode(select,OUTPUT);
#endif
      pad::type = detect();
  }
  
  void hc165n_latch(void){
    digitalWrite(latch,LOW);_delay_us(5);digitalWrite(latch,HIGH);_delay_us(5);
  }
  
  void hc165n_clockJump(u8 n){
    while(n--){
      digitalWrite(clock,HIGH);
      _delay_us(5);
      digitalWrite(clock,LOW);
      _delay_us(5);
    };
  }  
  
  void hc165n_serialRead8(u8*k){
    hc165n_latch();
    *k = digitalRead(data);
    u8 n=7;
    while(n--){
      hc165n_clockJump(1);
      *k = (*k << 1) | digitalRead(data);
    };
  }

#ifndef pad_noSelectPin
  void setSelectPin(u8 v){
    digitalWrite(select,v);_delay_us(5);
  }
  void flashSelectPin(void){
    //digitalWrite(select,HIGH);_delay_us(5);digitalWrite(select,LOW);
    setSelectPin(HIGH);setSelectPin(LOW);
  }
#endif

  u8 detect(void){
#ifdef pad_noSelectPin
    return 2; // no select pin, 2 buttons
#else
    flashSelectPin(); // cycle 1,2
    u8 k; hc165n_serialRead8(&k); if((k>>2)&3) return 2; // no pad or master system one
    // left/right at the same time, genesis pad
    flashSelectPin(); // cycle 3,4
    flashSelectPin(); // cycle 5,6
    hc165n_serialRead8(&k); if(k&15) return 3; // 3 buttons pad
    flashSelectPin(); // cycle 7,8
    return 6; // 6 buttons pad
#endif
  }
  
  void update(void){
	keys.raw = 0xffff;
#ifndef pad_noSelectPin
	setSelectPin(HIGH); // cycle 1, arrow and master system 1.2 / genesis B.C
#endif
	hc165n_serialRead8(&keys.byte1);
#ifndef pad_noSelectPin
	if(type == 2) return; // master system
	// cycle 2, genesis A/Start
	setSelectPin(LOW);
	hc165n_latch();
	hc165n_clockJump(2);
	keys.start = digitalRead(data);
	hc165n_clockJump(1);
	keys.a = digitalRead(data);
	if(type == 3) return; // genesis 3 buttons
	// genesis 6 btn pad
	flashSelectPin(); // cycle 3,4
	flashSelectPin(); // cycle 5,6
	// cycle 7, genesis x/y/z
	setSelectPin(HIGH);
	hc165n_latch();
	hc165n_clockJump(4);
	keys.mode = digitalRead(data);
	hc165n_clockJump(1);
	keys.x = digitalRead(data);
	hc165n_clockJump(1);
	keys.y = digitalRead(data);
	hc165n_clockJump(1);
	keys.z = digitalRead(data);
	setSelectPin(LOW); // cycle 8
#endif
  }

};
