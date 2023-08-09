/*
2023.1.14
使用BusKeyBoard v1.1测试
按钮功能正常，注意到一个细节：A（拨轮开关）按压触发；BC释放触发。尽管程序写的是setPressedHandler。暂无大碍。

2023.1.22 尝试删掉抄来的GUI...

-23:41 截至目前，没有发现严重问题，此版通过，命名为1.0
*/

#include <U8g2lib.h>
#include <U8g2lib.h>
#include <Rotary.h>
#include <Button2.h>
#include <Keyboard.h>
#include <EEPROM.h>


#include "Wire.h"
#include "I2C_eeprom.h"

/* 150=30+60+60
<---title(30)---><---------user(60)---------><---------data(60)--------->
*/

#define BUTTON_PIN_A  5  // 拨轮开关上的
#define BUTTON_PIN_B  6  // 左按键
#define BUTTON_PIN_C  7  // 右按键
#define EEPROM_A_ADDR 0x50  // 第一颗EPPROM，标记丝印U2
#define EEPROM_B_ADDR 0x51  // 第二颗EPPROM，标记丝印U3

#define EVENT_NONE 0         // 用于切换上一个事件以避免无限循环
#define EVENT_ROTATE_CW 3    // 顺时针旋转旋转编码器
#define EVENT_ROTATE_CC 4    // 逆时针旋转旋转编码器
#define EVENT_ROTATE_NONE 5

Button2 Button_A = Button2(BUTTON_PIN_A);  
Button2 Button_B = Button2(BUTTON_PIN_B);  
Button2 Button_C = Button2(BUTTON_PIN_C);  

Rotary r = Rotary(9, 8);

I2C_eeprom eeprom_a(EEPROM_A_ADDR, I2C_DEVICESIZE_24LC256); // 0x50->A  0x51->B
I2C_eeprom eeprom_b(EEPROM_B_ADDR, I2C_DEVICESIZE_24LC256); // 0x50->A  0x51->B

I2C_eeprom ee = eeprom_a; // 使用A槽位

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);


/******************变量*********************/

char title[30];
char user[60];
char data[60];

int currentIndex = 0;
int lastIndex = 0; 

int dataNum = 0;

// ui_passwd()相关变量
int selectedIndex = 1;  // 1是第一位，首个分区（150 Bytes）被配置数据占了。
int lastSelectedIndex = 1;

volatile uint8_t event = EVENT_NONE;

/*******************************************/

// 拨轮中断
ISR(PCINT0_vect) {
  unsigned char result = r.process();
  Serial.println(result);
  if (result != DIR_NONE) {
    if (result == DIR_CW) {
      Serial.println("DIR_CW");
      currentIndex++;
      event = EVENT_ROTATE_CW;
    } else if (result == DIR_CCW) {
      Serial.println("DIR_CCW");
      currentIndex--;
      event = EVENT_ROTATE_CC;
    }
  }
  Serial.print("currentIndex: ");
  Serial.println(currentIndex);

}


void ConfReading() {
  dataNum = ee.readByte(0);
  Serial.print("DataNum: ");
  Serial.println(dataNum);      
}

void ConfWriting(int num) {
  Serial.println(num);
  Serial.println(ee.updateByte(0, num));    
}

void DataWriting(int index, char *title, char *user, char *data)
{
  if(index<=0){
    Serial.println("Error! Index must larger than 0");
  }else{
    for (int i = 0 ; i <= strlen(title) ; i++) { // <=是因为要把最后一位'\0'也写入
      Serial.print(ee.updateByte(150*index+i, title[i]));
    }Serial.println();
    for (int i = 0 ; i <= strlen(user) ; i++) { // <=是因为要把最后一位'\0'也写入
      Serial.print(ee.updateByte(150*index+i+30, user[i]));
    }Serial.println();
    for (int i = 0 ; i <= strlen(data) ; i++) { // <=是因为要把最后一位'\0'也写入
      Serial.print(ee.updateByte(150*index+i+90, data[i]));
    }Serial.println();
  }
}

void DataReading(int index)
{
  if(index<=0){
    Serial.println("Error! Index must larger than 0");
  }else{
    char buff;
    for (int i = 0; i <= 30; i++) {
      buff = ee.readByte(150*index+i);
      title[i] = buff;
      Serial.print(buff);    
      if(buff == '\0'){
        break;
      }
    }Serial.println();
    for (int i = 0; i <= 60; i++) {
      buff = ee.readByte(150*index+i+30);
      user[i] = buff;
      Serial.print(buff);
      if(buff == '\0'){
        break;
      }
    }Serial.println();
    for (int i = 0; i <= 60; i++) {
      buff = ee.readByte(150*index+i+90);
      data[i] = buff;
      Serial.print(buff);
      if(buff == '\0'){
        break;
      }
    }
  }
}

void TitleLoading(int index) {
  if(index<=0){
    Serial.println("Error! Index must larger than 0");
  }else{
    char buff;
    for (int i = 0; i <= 30; i++) {
      buff = ee.readByte(150*index+i);
      title[i] = buff;
      Serial.print(buff);    
      if(buff == '\0'){
        break;
      }
    }Serial.println();
  }
}

void UserLoading(int index) {
  if(index<=0){
    Serial.println("Error! Index must larger than 0");
  }else{
    char buff;
    for (int i = 0; i <= 30; i++) {
      buff = ee.readByte(150*index+i+30);
      user[i] = buff;
      Serial.print(buff);    
      if(buff == '\0'){
        break;
      }
    }Serial.println();
  }
}

void DataLoading(int index) {
  if(index<=0){
    Serial.println("Error! Index must larger than 0");
  }else{
    char buff;
    for (int i = 0; i <= 30; i++) {
      buff = ee.readByte(150*index+i+90);
      data[i] = buff;
      Serial.print(buff);    
      if(buff == '\0'){
        break;
      }
    }Serial.println();
  }
}

void RemoveLastSign() {
  ;
}


void ui_passwd(){
  u8g2.clearBuffer();         // 清除内部缓冲区

  if(event != EVENT_ROTATE_NONE) {
    if(event == EVENT_ROTATE_CW) {
      Serial.println("CW");
      selectedIndex++;
      if(selectedIndex > dataNum) {
        selectedIndex = dataNum;  
      }
    }else if(event == EVENT_ROTATE_CC) {
      Serial.println("CC");
      selectedIndex--;
      if(selectedIndex < 1) {
          selectedIndex = 1;  
      }
    }else {
      Serial.println("It's impossible!");
    }
  }event = EVENT_ROTATE_NONE;


  if(lastSelectedIndex != selectedIndex){
    Serial.println("!=");
    TitleLoading(selectedIndex);
  }
  
  // Serial.println("======Debug======");
  // Serial.println(selectedIndex);
  // Serial.println(lastSelectedIndex);
  // Serial.println(dataNum);
  // Serial.println("======Finish======");


  // Serial.println("Tick!");
  u8g2.drawStr(0,12,title);
  u8g2.sendBuffer();
  lastSelectedIndex = selectedIndex;
  
}



///////按键回调函数///////

void ButtonA() {
  Serial.println("A");
  UserLoading(selectedIndex);
  DataLoading(selectedIndex);

  Serial.println(user);
  Keyboard.print(user);

  Keyboard.press(KEY_TAB);
  Keyboard.release(KEY_TAB);

  Serial.println(data);
  Keyboard.print(data);

}

void ButtonB() {
  Serial.println("B");
  UserLoading(selectedIndex);
  Serial.println(user);
  Keyboard.print(user);
}

void ButtonC() {
  Serial.println("C");
  DataLoading(selectedIndex);
  Serial.println(data);
  Keyboard.print(data);
}

///////debug部分///////

String inputString;
bool stringComplete = false;

typedef void (*cmd_cb)(String*);
typedef struct
{
  String cmd;
  cmd_cb cb;
}ST_CMD;

void debugHelper()
{
  Serial.println("I'm Helper");
}
void debugWrite(String* cmd)
{
  Serial.println("Write!");
  Serial.println(cmd[1]);
  Serial.println(cmd[2]);
  Serial.println(cmd[3]);
  Serial.println(cmd[4]);
  int num = 0;
  sscanf(cmd[1].c_str(), "%d", &num);
  DataWriting(num, cmd[2].c_str(), cmd[3].c_str(), cmd[4].c_str());
  /*
  cmd[1]    int index
  cmd[2]    char * title
  cmd[3]    char * user
  cmd[4]    char * data
  */
}
void debugRead(String* cmd)
{
  Serial.println("Read!");
  int num = 0;
  sscanf(cmd[1].c_str(), "%d", &num);
  DataReading(num);  // -> index
}

void debugConfWrite(String* cmd)
{
  int num = 0;
  sscanf(cmd[1].c_str(), "%d", &num);
  ConfWriting(num);
}

void debugConfRead()
{
  ConfReading();
}

ST_CMD cmd_list[]
{
  {"?", debugHelper},
  {"write", debugWrite},
  {"read", debugRead},
  {"confwrite", debugConfWrite},
  {"confread", debugConfRead},
};

void debugProc() {
  inputString = '\0';String cmd[5];int count=0;char buf[64];int c=0;
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if(inChar==' '){
      count++;
    }else {
      cmd[count]+=inChar;
    }
    // add it to the inputString:
    inputString += inChar;
    buf[c]=inChar;
    c++;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
  if (stringComplete == true) {

    int i,size=sizeof(cmd_list)/sizeof(ST_CMD);
    for (i = 0; i < size; i++) {
      // Serial.print(cmd[0]);Serial.print(' ');
      if (cmd[0] == cmd_list[i].cmd) {
        Serial.println("OK");
        Serial.println(cmd[0]);
        Serial.println(cmd_list[i].cmd);
        cmd_list[i].cb(cmd);
        // break;
      }else {
        Serial.println("Fuck");
      }
    }
  }stringComplete = false;//Serial.println('.');
}

////////////////////


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(BUTTON_PIN_A, INPUT_PULLUP); // 设置上拉模式
  pinMode(BUTTON_PIN_B, INPUT_PULLUP);
  pinMode(BUTTON_PIN_C, INPUT_PULLUP);

  Button_A.setPressedHandler(ButtonA);
  Button_B.setPressedHandler(ButtonB);
  Button_C.setPressedHandler(ButtonC);

  r.begin();
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT5) | (1 << PCINT4);
  sei();

  u8g2.begin();
  u8g2.setFont(u8g2_font_t0_18_mf ); //设置字体


  ee.begin();
  if (! ee.isConnected())
  {
    Serial.println("ERROR: Can't find eeprom\nstopped...");
    while (1);
  }
  
  Keyboard.begin();

  ConfReading();
  TitleLoading(1);

}

void loop() {
  // put your main code here, to run repeatedly:
  ui_passwd();
  Button_A.loop();
  Button_B.loop();
  Button_C.loop();
  debugProc();

}
