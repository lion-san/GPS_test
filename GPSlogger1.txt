/*******************************************************************************
*  GPSlogger1.ino - ＧＰＳ(GMS6-CR6)からのメッセージをＳＤに記録するサンプル１ *
*                                                                              *
*    メモ - LEDがOFF時(停止中)にSDの抜き差しを行った方が良いでしょう。         *
*           又、電源を切る場合も一旦"停止中"にしてから行いましょう。           *
*           スイッチサイエンスの「マイクロＳＤシールド」を利用しています、     *
*           よってデジタルピン 8,10,11,12,13 は使用出来ないです。              *
*           EEPROMの０番地をファイル名の可変データとして使用します。           *
*                                                                              *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2014-08-20  きむ茶工房(きむしげ)  Create                            *
* ============================================================================ *
* Arduino Duemilanove Uno R3                                                   *
* Arduino IDE V1.0.5-r2                                                        *
*******************************************************************************/
#include <string.h>
#include <EEPROM.h>
#include <SD.h>
#include <SoftwareSerial.h>

#define SW_PIN_NO             6         // スイッチの接続ピン番号
#define LED_PIN_NO            7         // LEDの接続ピン番号
#define SENTENCES_BUFLEN      82        // GPSのメッセージデータバッファの個数
#define WRITE_INTERVAL        3         // センテンスを書き込む間隔の指定

int  RunFlag ;                          // 1:書込み起動中  0:停止中
int  WriteIntervalCount ;               // センテンスを書き込む間隔のカウンタ
byte SentencesData[SENTENCES_BUFLEN] ;  // GPSのセンテンスデータバッファ
int  SentencesNum ;                     // GPSのセンテンス文字列個数
int  GPS_Status ;                       // GPSの受信状態 0:無効 1:有効

struct {                                // 受信したセンテンスの書き込み情報
  char sentencesID[7] ;                 // $<TalkerID:2><センテンスID;3>
  int  WriteFlag ;                      // 1:書き込む  0:書き込まない
} WritingInfo[] = {                     // "$GPGGA"と"$GPRMC"は書込み必須です
  {"$GPGGA",1} ,{"$GPGSA",0} ,{"$GPRMC",1} ,
  {"$GPGLL",0} ,{"$GPGSV",0} ,{"$GPVTG",0} ,{"$GPZDA",0} ,
} ;
char Filename[13] = "G0000000.TXT" ;    // ファイル名 = G[$GPRMCの日付][A-Z].TXT

// GPS通信のソフトシリアルライブラリを生成する
SoftwareSerial GPSserial(2, 3) ;        // 受信:RX(2) 送信:TX(3)


/*******************************************************************************
*  電源起動時とリセットの時だけのみ処理される関数(初期化と設定処理)            *
*******************************************************************************/
void setup()
{
    int x ;

     pinMode(LED_PIN_NO,OUTPUT) ;       // LEDに接続
     pinMode(SW_PIN_NO,INPUT_PULLUP ) ; // SW に接続し内部プルアップに設定
     // IDEとの通信を初期化する
     Serial.begin(9600) ;
     // ＳＤカードの初期化処理(フォーマットではないよ)
     pinMode(10, OUTPUT) ;              // この行がないとライブラリが動作しないらしい
     if (!SD.begin(8)) {
         Serial.println("Card failed, or not present") ; // 初期化に失敗(SD未挿入かも)
     } else Serial.println("card initialized.") ;        // 初期化完了
     // GPSとの通信を初期化する
     GPSserial.begin(4800) ;
     // ファイル名の可変部分をEEPROMに設定する
     // ０番地が"A"-"Z"のデータならそのままで違うなら"A"で初期化する
     x = EEPROM.read(0) ;
     if (!(x >= 'A' && x <= 'Z')) EEPROM.write(0,'A') ;
     // 変数類の初期化を行う
     WriteIntervalCount = 0 ;
     SentencesNum = -1 ;
     GPS_Status   = 0 ;
     RunFlag      = 0 ;
}
/*******************************************************************************
*  繰り返し実行される処理の関数(メインの処理)                                  *
*******************************************************************************/
void loop()
{
     int  i ;
     char dt ;

     // スイッチのON/OFFにより起動状態を変える処理
     if (digitalRead(SW_PIN_NO) == LOW) {
          RunFlag = ~RunFlag ;
          if (RunFlag == 0) {
               Serial.println("OFF") ;
               digitalWrite(LED_PIN_NO,LOW) ;     // 停止中
          } else {
               Serial.println("ON") ;
               digitalWrite(LED_PIN_NO,HIGH) ;    // 起動中
               // ファイルの可変部分を取出しファイル名にセットする
               i = EEPROM.read(0) ;
               Filename[7] = (char)i ;
               // ファイル名の可変部分は次の文字に進めてEEPROMに保存
               i++ ;
               if (i > 'Z') i = 'A' ;
               EEPROM.write(0,i) ;
          }
          delay(1000) ;                           // チャタリング防止
     }
     if (RunFlag == 0) return ;                   // 停止中なら以降処理しない

     // センテンスデータが有るなら処理を行う
     if (GPSserial.available()) {
          // 1バイト読み出す
          dt = GPSserial.read() ;
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
/*******************************************************************************
*  GPS_WriteMessage()                                                          *
*    センテンスをＳＤに書き込む処理                                            *
*    ファイルが無い時は作成され、有ればファイルの最後に追加されます            *
*******************************************************************************/
void GPS_WriteMessage()
{
     File fds  ;

     // ファイルの書込みオープン
     fds = SD.open(Filename,FILE_WRITE) ;
     if (fds) {
          fds.write(SentencesData,SentencesNum) ;
          fds.close() ;     // ファイルのクローズ
     } else {
          // ファイルのオープンエラー
          Serial.println("error opening") ;
     }
}
/*******************************************************************************
*  GPS_StatusCheck()                                                           *
*    GPSのセンテンスが有効か調べる処理                                         *
*    "$GPRMC"メッセージの2項目の文字列で判断します。                           *
*******************************************************************************/
void GPS_StatusCheck()
{
     char buf[4] ;
     int  ans ;
     
     memset(buf,0x00,sizeof(buf)) ;
     ans = GPS_GetMessageItem(buf,"$GPRMC",2) ;
     if (ans == 1 && buf[0] == 'A') {
          // 日付(UTC)をファイル名にセットする
          GPS_GetMessageItem(&Filename[1],"$GPRMC",9) ;
          // ステータスは"有効"
          GPS_Status = 1 ;
     }
}
/*******************************************************************************
*  ans = GPS_GetMessageItem(*dt1,*dt2,cnt)                                     *
*    GPSから受信したセンテンスの各項目を取り出す処理                           *
*    受信したメッセージが *dt2 と一致したら ans=1 でパラメータを取り出します。 *
*    指定した位置(cnt)のパラメータ文字列を取り出して *dt1 に内容を設定します。 *
*                                                                              *
*    dt1 : 取り出したセンテンス項目(パラメータ)の文字列を返す                  *
*          取り出す文字列を格納するバッファは0x00で初期化して置いた方が良いです*
*    dt2 : $<vendor:2><message;3>を指定、例えば"$GPRMC"等です                  *
*    cnt : 取り出すパラメータの位置を指定します                                *
*          $GPRMC,075453.000,A,･････*68でcnt=2ならdt1="A"となりヌルは付かない。*
*    ans : 指定の*dt2 と一致したらans=1で不一致ならans=0                       *
*******************************************************************************/
int GPS_GetMessageItem(char *dt1,char *dt2,int cnt)
{
     char *p ;
     int ans , i , j , c ;

     ans = 0 ;
     // 受信したセンテンスは、指定のメッセージと一致したか調べる
     if (strncmp((char *)SentencesData,dt2,6) == 0) {
          ans = 1 ;
          c   = 0 ;
          // センテンスの長さだけ繰り返す
          for (i=0 ; i<SentencesNum ; i++) {
               if (SentencesData[i] == ',') c++ ; // 区切り文字を数える
               if (c == cnt) {
                    // 指定した場所の区切り文字ならそこのパラメータをコピーする
                    p = (char *)dt1 ;
                    for (j=i+1 ; j<SentencesNum ; j++) {
                         if (SentencesData[j]==',' || SentencesData[j]=='*') break ;
                         *p = SentencesData[j] ;
                         p++ ;
                    }
                    break ;
               }
          }
     }
     return ans ;
}
