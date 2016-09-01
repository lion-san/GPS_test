#include <SoftwareSerial.h>

#define RX 10
#define TX 11

SoftwareSerial  g_gps( RX, TX );

void setupSoftwareSerial(){
  g_gps.begin(9600);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  while(!Serial){
    ;
  }

  Serial.println("GPS Start");

  setupSoftwareSerial();

}

void loop() {
  // put your main code here, to run repeatedly:
  if(g_gps.available()){
    Serial.write(g_gps.read());

    Serial.println("GPS OK");
  }
  
  Serial.println("GPS Wait");

  delay(1000);

}
