#include <SoftwareSerial.h>

#define RX 8
#define TX 9

#define SENTENCES_BUFLEN      82        // GPSのメッセージデータバッファの個数

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

  setupSoftwareSerial();


  Serial.println("GPS Start");


}

void loop() {
  // put your main code here, to run repeatedly:
  //if(g_gps.available()){
  //  Serial.write(g_gps.read());

  //  Serial.println("GPS OK");
  //}

  receiveGPS();
  
  Serial.println("GPS Wait");

  else{
  
    //Serial.println("GPS Wait");
  }
  //delay(1000);

}


/**
 * receiveGPS
 */
String receiveGPS(){

  int SentencesNum = 0;                   // GPSのセンテンス文字列個数
  byte SentencesData[SENTENCES_BUFLEN] ;  // GPSのセンテンスデータバッファ
  char dt ;

   // センテンスデータが有るなら処理を行う
   if (g_gps.available()) {
        // 1バイト読み出す
        dt = g_gps.read() ;
        // センテンスの開始
        if (dt == '$') SentencesNum = 0 ;
        
        if (SentencesNum >= 0) {
          
             // センテンスをバッファに溜める
             SentencesData[SentencesNum] = dt ;
             SentencesNum++ ;
             
             // センテンスの最後(LF=0x0Aで判断)
             if (dt == 0x0a || SentencesNum >= SENTENCES_BUFLEN) {
                  // センテンスのステータスが"有効"になるまで待つ
                  if (GPS_Status == 0) GPS_StatusCheck() ;
                  else {
                       // 有効になったら書込み開始
                       for (i=0 ; i<7 ; i++) {
                            // 受信したセンテンスが書込み対象か調べる
                            if (strncmp((char *)SentencesData,WritingInfo[i].sentencesID,6) == 0 &&
                                WritingInfo[i].WriteFlag == 1) {
                                 // 書込む間隔が来たら書き込む
                                 if (i==0) WriteIntervalCount++ ;
                                 if (WriteIntervalCount == WRITE_INTERVAL) {
                                      //Serial.write(SentencesData,SentencesNum) ;
                                      GPS_WriteMessage() ;  // SD書込み
                                 }
                                 // 書き込む間隔のカウンタをリセット
                                 if (WriteIntervalCount > WRITE_INTERVAL) {
                                      WriteIntervalCount = 0 ;
                                 }
                                 break ;
                            }
                       }
                  }
                SentencesNum = -1 ;
             }
        }
   }
 
}

