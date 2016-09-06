#include <SoftwareSerial.h>

#define RX 8
#define TX 9

#define SENTENCES_BUFLEN      82        // GPSのメッセージデータバッファの個数

//=== Global ===========================================
SoftwareSerial  g_gps( RX, TX );
char head[] = "$GPRMC";
char info[] = "$GPGGA";
char buf[10];
int SentencesNum = 0;                   // GPSのセンテンス文字列個数
byte SentencesData[SENTENCES_BUFLEN] ;  // GPSのセンテンスデータバッファ
//======================================================


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
  char dt ;

   // センテンスデータが有るなら処理を行う
   if (g_gps.available()) {
        // 1バイト読み出す
        dt = g_gps.read() ;
        //Serial.write(dt);//Debug ALL
        // センテンスの開始
        if (dt == '$') SentencesNum = 0 ;
        
        if (SentencesNum >= 0) {
          
          // センテンスをバッファに溜める
          SentencesData[SentencesNum] = dt ;
          SentencesNum++ ;
             
          // センテンスの最後(LF=0x0Aで判断)
          if (dt == 0x0a || SentencesNum >= SENTENCES_BUFLEN) {

            SentencesData[SentencesNum] = '\0' ;

            //GPS情報の取得
            getGpsInfo();

            
            // センテンスのステータスが"有効"になるまで待つ
            if ( gpsIsReady() )
            {
               // 有効になったら書込み開始
               Serial.print("O:");
               Serial.print( (char *)SentencesData );
            }
          }
        }
   }
  
  //delay(1000);

}

/**
 * getGpsInfo
 */
void getGpsInfo()
{
    int i, c;
    
    //$1ヘッダが一致
    if( strncmp((char *)SentencesData, info, 6) == 0 )
    {

      //コンマカウント初期化
      c = 1; 

      // センテンスの長さだけ繰り返す
      for (i=0 ; i<SentencesNum; i++) {
        if (SentencesData[i] == ','){
          
            c++ ; // 区切り文字を数える
    
            if ( c == 2 ) {
                 Serial.println("----------------------------");
                // Serial.println((char *)SentencesData);
                 Serial.print("Time:");
                 Serial.println(readDataUntilComma(i+1));
                 continue;
            }
            else if ( c == 8 ) {
                // Serial.println((char *)SentencesData);
                 Serial.print("Number of Satelites:");
                 Serial.println(readDataUntilComma(i+1));
                 continue;
            }
        }
      }
      
    }
}

/**
 * receiveGPS
 */
/*char *receiveGPS()
{
  char dt ;
  
  //初期化
  memset(SentencesData, 0x00, SENTENCES_BUFLEN) ;

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
 
}*/

/**
 * gpsStatusCheck
 */
boolean gpsIsReady()
{
    int i, c;
    
    //$1ヘッダが一致かつ,$3ステータスが有効＝A
    if( strncmp((char *)SentencesData, head, 6) == 0 )
    {

      //コンマカウント初期化
      c = 1; 

      // センテンスの長さだけ繰り返す
      for (i=0 ; i<SentencesNum; i++) {
        if (SentencesData[i] == ','){
              
              c++ ; // 区切り文字を数える
    
            if ( c == 3 ) {
                 //次のコンマまでのデータを呼び出し
                 if( strncmp("A", readDataUntilComma(i+1), 1) == 0 ){
                   return true;
                 }
                 else{
                   Serial.print("X:");
                   Serial.print( (char *)SentencesData );
                   return false;
                 }
            }
        }
      }
      
    }

    return false;
}

/**
  * readDataUntilComma
  */
char* readDataUntilComma(int s)
{
  int i, j;

  j = 0;
  //初期化
  memset(buf,0x00,sizeof(buf)) ;

  //終了条件
  //次のコンマが出現or特定文字*（チェックサム)が出現
  for (i = s; i < SentencesNum; i++)
  {
    if(( SentencesData[i] == ',') || (SentencesData[i] == '*')){
      buf[j] = '\0';
      return buf;
    }
    else{
      //バッファーのオーバフローをチェック
      if( j < 10 ) {
        buf[j] = SentencesData[i];
        j++;
      }
      else{//エラー処理
        int x;
        for(x = 0; x < sizeof(buf); x++)
          buf[x] = 'X';
          return buf;
      }
      
    }
  }
  
}
