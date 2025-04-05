#define VER 1.01
#define SKETCH_NAME "BlindController"

//#define DEV
//#define RADIO

// define I/O pins
  // I/O pin is the same as child sensor
  #define BLINDS_UP 7
  #define BLINDS_STOP 6
  #define BLINDS_DOWN 5
  #define BLINDS_CHANNEL_PLUS 4 // I/O pin
  #define BLINDS_CHANNEL 4      // child sensor: channel from HA
  #define BLINDS_SEGMENT_e 3 
  
  #ifdef DEV
    #define OSCILLOSCOPE_TRIGGER 2 // output channel to trigger oscilloscope
  #endif

  uint8_t Opin[4] = {BLINDS_CHANNEL_PLUS, BLINDS_DOWN, BLINDS_STOP, BLINDS_UP} ;  // sensors are indexed 0 through 3
  volatile bool sensorState[4] = {false,false,false,false};
  volatile uint8_t incomingChannel;  // channel of remote control to use
  uint8_t channel;
  unsigned long TimeOfLastButtonPressed = millis();
  #define ButtonPressTimout 4500 // 4-1/2 seconds

// define operational constants
  #define DWELL_TIME 300 // mSec to hold button down
  #define BUTTON_PUSHED 1
  #define BUTTON_NOT_PUSHED 0

//------------------------------------------------------------------------ infrastructure

#ifdef RADIO
  // Enable and select radio type attached
  #define MY_RADIO_RF24
  #define MY_RF24_CS_PIN 9
  #define MY_RF24_CE_PIN 10
  #ifdef DEV
    // Enable debug prints to serial monitor
    #define MY_DEBUG
    #define MY_RF24_PA_LEVEL (RF24_PA_MIN)
    #define MY_RF24_CHANNEL 86
  #else
    #ifdef MY_DEBUG
      #undef MY_DEBUG
    #endif
    #define MY_RF24_PA_LEVEL (RF24_PA_MAX)
    #define MY_RF24_CHANNEL 121
  #endif 
#else
  // Enable serial gateway
  #define MY_GATEWAY_SERIAL
  #ifdef DEV
    #define MY_DEBUG
  #else
    #undef MY_DEBUG
  #endif
#endif

//------------------------------------------------------------------------ MySensors
#include <MySensors.h>
#ifdef RADIO
  #define MESSAGE_WAIT 500     // ms to wait between each send message
  #define REGISTERING_VARIABLES_WAIT 4000  // ver 2.01
#else
  #define MESSAGE_WAIT 100     // ms to wait between each send message
  #define REGISTERING_VARIABLES_WAIT 500  // ver 2.01
#endif

// BlindsUp
MyMessage msgBlindsUp(BLINDS_UP, V_STATUS);
// BlindsStop
MyMessage msgBlindsStop(BLINDS_STOP, V_STATUS);
// BlindsClose
MyMessage msgBlindsDown(BLINDS_DOWN, V_STATUS);
// BlindCha
MyMessage msgBlindsChannel(BLINDS_CHANNEL_PLUS, V_TEXT);
// 


//------------------------------------------------------------------------before
void before(){
  #ifdef DEV
    Serial.println(F("--------------------------- DEV ---------------------------"));
  #endif
  Serial.print(SKETCH_NAME);Serial.print(F("Version "));Serial.println(VER);

  // ensure all outputs are not pushing a button
  for (uint8_t sensor = 0;sensor<4;sensor++) { 
    pinMode(Opin[sensor],OUTPUT);
    digitalWrite(Opin[sensor],BUTTON_NOT_PUSHED);
  }

}
//------------------------------------------------------------------------presentation
void presentation(){
	// Send the sketch version information to the gateway and Controller
  char sVER[6];dtostrf(VER,9,2,sVER);
	sendSketchInfo(SKETCH_NAME, sVER);
  
  present(BLINDS_UP,S_BINARY,"BlindsUp");
  wait(REGISTERING_VARIABLES_WAIT);
  present(BLINDS_STOP,S_BINARY,"BlindsStop");
  wait(REGISTERING_VARIABLES_WAIT);
  present(BLINDS_DOWN,S_BINARY,"BlindsDown");
  wait(REGISTERING_VARIABLES_WAIT);
  present(BLINDS_CHANNEL_PLUS,S_INFO,"BlindsChannel");
  wait(REGISTERING_VARIABLES_WAIT);

}

//------------------------------------------------------------------------ SensorToSensorPtr
uint8_t SensorToSensorPtr(uint8_t child){
  for (uint8_t i = 0;i<4;i++) if (Opin[i] == child) return i;
  return 0xff; // in case it's an invalid sensor
}


//------------------------------------------------------------------------ChangeChannel
void ChangeChannel(uint8_t destination){
  if (channel == destination) return;  // it's already there
  if (millis() - TimeOfLastButtonPressed > ButtonPressTimout) PushButton(BLINDS_CHANNEL_PLUS);  // remote won't acknowledge channgel change if button timed out
  while ( channel != destination) {
    PushButton(BLINDS_CHANNEL_PLUS);
    channel = ++channel % 16;
  }
  #ifdef DEV
    MY_SERIALDEVICE.print("Channel is: ");MY_SERIALDEVICE.println(channel);
  #endif

}
//------------------------------------------------------------------------Segment_e_State
bool Segment_e_State(){
  bool lessThanTen = true;
  for (int i=0;i<4;i++) {
    if (! digitalRead(BLINDS_SEGMENT_e)) lessThanTen = false;
    delay(3);
  }
  return lessThanTen;
}
//------------------------------------------------------------------------SyncChannel
uint8_t SyncChannel(){
  PushButton(BLINDS_STOP); // ensures that the remote is awake 
                           // so that a channel take will takeeffect onfirst try
  bool eState = Segment_e_State();
  while (eState == Segment_e_State()) PushButton(BLINDS_CHANNEL_PLUS);
  PushButton(BLINDS_STOP);  // Sets the channel so that an UP or Down will take effect on first try
  if (eState) 
    return 0;
  else
    return 10;

}
//------------------------------------------------------------------------PushButton
void PushButton(uint8_t buttonToPush){
  digitalWrite(buttonToPush,BUTTON_PUSHED);
  delay(DWELL_TIME);
  digitalWrite(buttonToPush,BUTTON_NOT_PUSHED);
  TimeOfLastButtonPressed = millis();
  #ifdef DEV
    MY_SERIALDEVICE.print(F("Button "));MY_SERIALDEVICE.print(buttonToPush);MY_SERIALDEVICE.println(F(" pushed"));
  #endif
  delay(DWELL_TIME);
}

//------------------------------------------------------------------------setup
void setup() {

  // initialize input for sync'ing to remote control
  pinMode(BLINDS_SEGMENT_e,INPUT_PULLUP);
  #ifdef MY_DEBUG
    pinMode(OSCILLOSCOPE_TRIGGER,OUTPUT);
    digitalWrite(OSCILLOSCOPE_TRIGGER,0);
  #endif

  MY_SERIALDEVICE.println();
  MY_SERIALDEVICE.print(SKETCH_NAME);MY_SERIALDEVICE.print(F("Version "));MY_SERIALDEVICE.println(VER);

  channel = SyncChannel();

  #ifdef DEV
    MY_SERIALDEVICE.print("Channel is: ");MY_SERIALDEVICE.println(channel);
  #endif DEV

  // Inform HA of sensors
  send(msgBlindsUp.set(  (sensorState[SensorToSensorPtr(BLINDS_UP)]  ) ) );
  wait(MESSAGE_WAIT);
  send(msgBlindsStop.set((sensorState[SensorToSensorPtr(BLINDS_STOP)]) ) );
  wait(MESSAGE_WAIT);
  send(msgBlindsDown.set((sensorState[SensorToSensorPtr(BLINDS_DOWN)]) ) );
  wait(MESSAGE_WAIT);

  send( msgBlindsChannel.set( channel ) );
  wait(MESSAGE_WAIT);


}
#ifdef DEV
  volatile bool messageReceived = false;
  volatile uint8_t messageReceivedFrom = 0xff;
  volatile uint8_t messageRecivedFor = 0xff;
#endif
//------------------------------------------------------------------------loop
void loop() {
  #ifdef DEV
    if (messageReceived){
      messageReceived = false;
      MY_SERIALDEVICE.print(F("Message Received from "));
        MY_SERIALDEVICE.print(messageReceivedFrom);
          MY_SERIALDEVICE.print(F(" for "));
            MY_SERIALDEVICE.println(messageRecivedFor);
      messageReceivedFrom = 0xff;
      messageRecivedFor = 0xff;
    }
  #endif

  for (uint8_t sensorPtr = 0;sensorPtr<4;sensorPtr++){
    if (sensorState[sensorPtr]) {
      sensorState[sensorPtr] = false;  // reset value because button was pressed
      // inform HA that button was pressed
      switch (Opin[sensorPtr]){  
        case BLINDS_UP :
          PushButton(Opin[sensorPtr]);
          send(msgBlindsUp.set(sensorState[sensorPtr]));
          break;
        case BLINDS_STOP :
          PushButton(Opin[sensorPtr]);
          send(msgBlindsStop.set(sensorState[sensorPtr]));
          break;
        case BLINDS_DOWN :
          PushButton(Opin[sensorPtr]);
          send(msgBlindsDown.set(sensorState[sensorPtr]));
          break;
        case BLINDS_CHANNEL :
          ChangeChannel(incomingChannel);
          send(msgBlindsChannel.set(channel));
          break;
        default :
          break;
      }
      wait(MESSAGE_WAIT);
    }
  }

}

//------------------------------------------------------------------------receive
void receive(const MyMessage &message){
  // minimize time in interupt service routine 
  // Get message, flag, and leave ... no print statements
  #ifdef DEV
    messageReceived=true;
    messageReceivedFrom = message.getSender();
    messageRecivedFor = message.getSensor();
  #endif

	if (message.getSender() == 0 ) {  // Only listen to messages from controller
    uint8_t sensorIn = message.getSensor();  // sensor number which is allso the I/O pin of the sensor
    uint8_t sensorPtrIn = SensorToSensorPtr(sensorIn);
    if (sensorPtrIn >= 0 && sensorPtrIn < 4) 
      switch(sensorIn) {
        case BLINDS_UP :
        case BLINDS_STOP :
        case BLINDS_DOWN :
          sensorState[sensorPtrIn] = message.getBool();
          break;
        case BLINDS_CHANNEL :
          incomingChannel = message.getInt();
          if (incomingChannel == channel || incomingChannel < 0 || incomingChannel > 15 ) break; // nothing to change or invalid
          sensorState[sensorPtrIn] = true;
          break;
        default :
          break;
      }
  }
  

}
