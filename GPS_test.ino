#include <SoftwareSerial.h>

#define RX 8
#define TX 9

#define SENTENCES_BUFLEN      82        // GPSのメッセージデータバッファの個数

SoftwareSerial  g_gps( RX, TX );
char head[] = "$GPRMC";

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

  //receiveGPS();

  int SentencesNum = 0;                   // GPSのセンテンス文字列個数
  char SentencesData[SENTENCES_BUFLEN] ;  // GPSのセンテンスデータバッファ
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
            if ( gpsIsReady(SentencesData) )
            {
               // 有効になったら書込み開始
               Serial.println( SentencesData );
            }
          }
        }
   }
  
  //delay(1000);

}


/**
 * receiveGPS
 */
char *receiveGPS()
{

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
               if ( gpsIsReady(SentencesData) )
               {
                  // 有効になったら書込み開始
                  return (char *)SentencesData;
               }
          }
        }
   }
 
}

/**
 * gpsStatusCheck
 */
boolean gpsIsReady(byte *data)
{
    int i, c;
      
    //$1ヘッダが一致かつ,$3ステータスが有効＝A
    if( strncmp((char *)data, head, 6) == 0 )
    {

      //コンマカウント初期化
      c = 1; 

      // センテンスの長さだけ繰り返す
      for (i=0 ; i<SentencesNum; i++) {
        if (data[i] == ',') c++ ; // 区切り文字を数える

        if ( c == 3 ) {
             //次のコンマまでのデータを呼び出し
             if( 'A' == readDataUntilComma(i, data) ){
               return true;
             }
             else
               return false;
        }
      }
    }

    return false;
}

/**
  * readDataUntilComma
  */
char *readDataUntilComma(int s, char *data)
{
  char buf[10];
  int i, j;

  j = 0;
  //初期化
  memset(buf,0x00,sizeof(buf)) ;

  //終了条件
  //次のコンマが出現or特定文字*（チェックサム)が出現
  for (i = s; i < SentencesNum; i++)
  {
    if(( data[i] == ",") || (data[i] == "*")){
      return buf;
    }
    else{
      //バッファーのオーバフローをチェック
      if( j < 10 ) {
        buf[j] = data[i];
        j++;
      }
    }
  }
  
}
