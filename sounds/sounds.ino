#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <Line.h> // for smooth transitions
#include <RollingAverage.h>
#include <ControlDelay.h>
#include <tables/triangle_warm8192_int8.h> // triangle table for oscillator
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <mozzi_midi.h>
#include <twi_nonblock.h>

#define ACC_IDLE 0
#define ACC_READING 1
static volatile byte acc_status = 0;

unsigned char sensor1 = 0;
unsigned char sensor2 = 0;

#define CONTROL_RATE 64 // powers of 2 please

Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> aTriangle(TRIANGLE_WARM8192_DATA);
Line <long> aGliss;
int glisses_per_second = 1 << 4;
byte lo_note = 24; // midi note numbers
byte hi_note = 36;
byte gliss_offset_min = 0;
byte  gliss_offset_max = 64;
int counter = 0;

unsigned int echo_cells_1 = 32;
unsigned int echo_cells_2 = 60;
unsigned int echo_cells_3 = 127;
ControlDelay <128, int> kDelay; // 2seconds
// oscils to compare bumpy to averaged control input
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin0(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);
// use: RollingAverage <number_type, how_many_to_average> myThing
RollingAverage <int, 32> kAverage; // how_many_to_average has to be power of 2
int averaged;

void setup() {
    Serial.begin(9600);
    initialize_twi_nonblock();
    startMozzi(CONTROL_RATE);

    kDelay.set(echo_cells_1);
}

void loop() {
  audioHook();
}

void updateControl(){
   switch( acc_status ){
  case ACC_IDLE:
//    Serial.print(int(sensor1));
//    Serial.print("\t");
//    Serial.println(int(sensor2));
    
    if (twi_initiateReadFrom(8, 2) != 0) {
        Serial.println("COMM FAILURE");
        break;
    }
    acc_status = ACC_READING;
    break;
  case ACC_READING:
    if ( TWI_MRX != twi_state ){
      byte read = twi_readMasterBuffer( rxBuffer, 2 );
      
      if (read < 2) {
        Serial.println("COMM FAILURE 2");
        break;
      } else {
        Serial.print(int(sensor1));
        Serial.print("\t");
        Serial.println(int(sensor2));
      }

      sensor1 = rxBuffer[0];
      sensor2 = rxBuffer[1];
    
      acc_status = ACC_IDLE;
    }
    break;
  }

  averaged = kAverage.next(sensor1);
  aSin0.setFreq(averaged);
  aSin1.setFreq(kDelay.next(averaged));
  aSin2.setFreq(kDelay.read(echo_cells_2));
  aSin3.setFreq(kDelay.read(echo_cells_3));
  
  if (--counter <= 0){
    byte gliss_offset = constrain(sensor2 >> 2, gliss_offset_min, gliss_offset_max);
    
    long audio_steps_per_gliss = AUDIO_RATE / glisses_per_second;
    long control_steps_per_gliss = CONTROL_RATE / glisses_per_second;
    
    // only need to calculate frequencies once each control update
    int start_freq = mtof(lo_note+gliss_offset);
    int end_freq = mtof(hi_note+gliss_offset);
    
    // find the phase increments (step sizes) through the audio table for those freqs
    // they are big ugly numbers which the oscillator understands but you don't really want to know
    long gliss_start = aTriangle.phaseIncFromFreq(start_freq);
    long gliss_end = aTriangle.phaseIncFromFreq(end_freq);
    
    // set the audio rate line to transition between the different step sizes
    aGliss.set(gliss_start, gliss_end, audio_steps_per_gliss);
    
    counter = control_steps_per_gliss;
  }
}

int updateAudio(){
  aTriangle.setPhaseInc(aGliss.next());

  // -132 129
  int echo = 3*((int)aSin0.next()+aSin1.next()+(aSin2.next()>>1)
    +(aSin3.next()>>2))>>3;
  // -101 127
  int gliss = aTriangle.next();

  if (sensor1 == 0) {
    echo = 0;
  }
  if (sensor2 == 0) {
    gliss = 0;
  }
  return (echo + gliss) >> 1;
}
