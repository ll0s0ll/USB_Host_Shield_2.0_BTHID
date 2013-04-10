# USB Host Library Rev.2.0 BTHID

The code is released under the GNU General Public License.

## 概要
USB Host Library for ArduinoをBluetooth HIDに特化させたものです。USB\_Host\_Shield\_2.0リポジトリよりフォークして作られました。  USB\_Host\_Shield\_2.0については下記をご覧ください。  
  
USB\_Host\_Shield\_2.0 リポジトリ  
<http://github.com/felis/USB\_Host\_Shield\_2.0>  
プロジェクトサイト   
<http://www.circuitsathome.com>

Bluetooth HIDは一般的な規格ですが、ArduinoからiPadへ文字を入力することを目的として開発されたため、現時点では一部のBluetooth HID対応デバイスでしか動作しません。今後はPCなどでも使えるようになればよいなと考えています。  
  
USB\_Host\_Shield\_2.0\_BTHID リポジトリ  
<http://github.com/ll0s0ll/USB\_Host\_Shield\_2.0\_BTHID>  
プロジェクトサイト  
<http://ll0s0ll.wordpress.com/>  

## 必要なもの
* Arduino（Uno R1を使ってテストしました）  
* USB Host Shield  
* Bluetoothドングル（PLANEX社のBT-MicroEDR1Xを使ってテストしました。他のドングルも使えるかもしれません）  
* iPad（3rdモデルでテストしました）

## 使い方
###Arduino IDE にライブラリをとりこむ  
USB Host Library Rev.2.0 BTHIDは、Arduino IDEで使えるライブラリになっています。
    
1. 下記URLよりライブラリをダウンロードし、解凍します。  
<http://github.com/ll0s0ll/USB_Host_Shield_2.0_BTHID/archive/master.zip>  
  
2. Arduino IDEではライブラリ名に「.」が使えませんので、「USB\_Host\_Shield\_20\_BTHID」等にフォルダ名を変更します。  
  
3. Arduino IDEを起動し、メニューから環境設定を開きます。環境設定の「スケッチブックの保存場所」を控えます。  
  
4. 「スケッチブックの保存場所」に記載されたフォルダをFinder等で開きます。そのフォルダの中に「libraries」というフォルダがあれば、そこにダウンロード&リネームしたフォルダをいれます。「USB\_Host\_Shield\_20\_BTHID」にリネームした場合、 下記のような階層構造になります。 
  * Arduino/
  	  * libraries/
		  * USB\_Host\_Shield\_20\_BTHID/  
5. Arduino IDEを再起動します。メニューの ファイル>スケッチの例 に、USB\_Host\_Shield\_20\_BTHID が出てくれば成功です。
    
###ライブラリの使い方  
ライブラリの使い方を知るためには、スケッチの例を見るのが一番です。メニューから ファイル＞スケッチの例＞USB\_Host\_Shield\_20\_BTHID＞BTHIDを開きます。基本的な部分は下記の4つです。
    
1. 必要なファイルをインクルードします。  
`#include <BTHID.h>` 
  
2. クラスのインスタンス化をします。  
`USB Usb;`  
`BTD Btd(&Usb);`  
`BTHID bthid(&Btd);`  

3. setup()でUSB部を初期化し、デバイス名をつけます。  
`Usb.Init();  //返り値が-1の場合はエラー   `   
`Btd.btdName = "Arduino";  //任意の名前を設定  `  
  
4. loop()内で思う存分コードを書く。  
`Usb.Task();  //USB部分の制御のためコールが必要`  
  
用意されている関数は５つです。これらの関数に送信したいデータを投げることで、Bluetooth HIDを通してデバイスにデータが送られます。データの型によって使い分けてください。  コードの例はBTHIDクラスをbthidという名前でインスタンス化した場合を表しています。
  
1. `void HID_sendString(char *str);`  
文字列を送信する関数です。最大32文字まで一度に送信できます。(最大値は機種によって異なります）ASCIIに準じた文字を送信できます。
  
 コードの例   
 `char str[] = "Hello World!";  //送信したい文字列を作成`  
 `bthid.HID_sendString(str);  //「Hello World!」と送信`  
   
2. `void HID_sendInteger(int16_t integer);`  
整数型を送信する関数です。メモリを食います。ビット数が少なくてよい場合はもう少し節約できるかもしれません。  
  
 コードの例    
 `bthid.HID_sendInteger(12345);  //12345を送信`  
  
3. `void HID_sendFloat(float myfloat);`  
浮動小数点型を送信する関数です。  桁数は調整すればもう少し増えるかもしれません。  
  
 コードの例  
 `bthid.HID_sendFloat(3.1415);  //3.1415を送信` 
  
4. `void HID_SendCharacter(uint8_t ascii);`  
ASCIIコードを変換して送信する関数です。ASCIIコードを投げてください。変換部分は制御文字を除いた部分を実装しています。（CRはReturn、SPはSpace、DELはDeleteに実装しています）  
  
 コードの例  
 `bthid.HID_SendCharacter(0x7F);   //0x7F = DEL(Delete)を送信`  
  
5. `void HID_sendKeyCodes(uint8_t modifier, uint8_t keycode1, uint8_t keycode2, uint8_t keycode3, uint8_t keycode4, uint8_t keycode5, uint8_t keycode6);`  
キーコードを送信する関数です。同時に6キーを入力できるようになっています。（同時に入力できる数はデバイスによって異なります）この関数を呼ぶとキーが押しっぱなしになっている状態になりますので、必ず`void HID_allKeyUp()`と併用するか、0x00のキーコードを送信してください。  
  
 キーコードはASCIIコードと異なるもので、USB接続のキーボード等で使われる規格です。詳しい値はUSB.orgから発行されている「Universal Serial Bus HID Usage Tables」の10章「Keyboard/Keypad Page (0x07)」（53ページ以降）に記載されています。 Usage IDをキーコードと読み替えてください。
    
 `uint8_t modifier`は修飾キー用の変数です。キーコード0xE0から0xE7までのキーが対象です。（0xE0=LeftControl、0xE1=LeftShift、0xE2=LeftAlt、0xE3=Left GUI、0xE4=RightControl、0xE5=RightShift、0xE6=RightAlt、0xE7=Right GUI）0xE0から0x01となるため、LeftShiftキーを押したい場合は`modifier = 0x02`となります。
  
 コードの例  
 `bthid.HID_sendKeyCodes(0x02,0x04,0x00,0x00,0x00,0x00,0x00);  //'A'を入力（0x02=LeftShift and 0x04='a'）`  
 `delay(5);  //少し時間を置きます`  
 `bthid.HID_allKeyUp();            // キーを放す`  
 `delay(5);  //少し時間を置きます`  
  
###スケッチの例について  
スケッチの例は、Arduino IDEのシリアルモニタを通して送られた文字によって、異なった関数が動くようになっています。  使い方は下記のようになります。  
  
1. Arduino IDEを起動し、メニューから ファイル＞スケッチの例＞USB\_Host\_Shield\_20\_BTHID＞BTHID を開く
2. ファイル＞マイコンボードに書き込む でプログラムをArduinoに書き込む。  
3. ツール＞シリアルモニタ でシリアルモニタを開く。  
4. シリアルモニタに「Bluetooth HID」と表示されて、処理が始まる。「OSC did not start」と表示される場合は、リセットボタンを押したりUSBケーブルをさし直してみたりする。  
5. 「Wait For Incoming Connection Request」と表示されたら検出可能状態になったので、iPad等のデバイスからペアリング操作をする。  
6. ペアリングが完了したら、シリアルモニタに目的の文字を入力して、送信ボタンを押す。  
7. 入力した文字に応じて、接続したデバイスに下記のように入力される。  
 * 1を入力した場合は「Hello World!」と入力される  
 * 2を入力した場合は「12345」と入力される  
 * 3を入力した場合は「3.1415」と入力される  
 * 4を入力した場合は、EISUキーが入力される（日本語環境のみ）  
 * 5を入力した場合は、KANAキーが入力される（日本語環境のみ）  
 * Qを入力した場合は、接続を切断する  
 * Dを入力した場合は、Deleteが入力される
 * Rを入力した場合は、Returnが入力される
8. その他のキーを入力した場合は、打ち込んだキーが入力される（ASCIIに対応したもキーのみ）  

##参考資料
ASCIIコードが掲載されています。  
Arduino - ASCIIchart  
<http://arduino.cc/en/Reference/ASCIIchart>　　
  
Apple Pro Keyboard JIS配列のキーコードが掲載されています。  
森山 将之のホームページ - USBキーボードのキーコード  
<http://www2d.biglobe.ne.jp/~msyk/keyboard/layout/usbkeycode.html>  
  
キーコードが記載されています。  
USB.org - Universal Serial Bus HID Usage Tables  
<http://www.usb.org/developers/devclass_docs/Hut1_11.pdf>  
  
PICマイコンを使ってBluetoothドングルをコントロールする方法がたいへん詳しく書かれています。  
辻見裕史のホームページ - PIC  
<http://phys.sci.hokudai.ac.jp/LABS/yts/pic/pic.html>