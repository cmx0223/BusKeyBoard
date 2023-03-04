/*
2023.1.14
使用BusKeyBoard v1.1测试
按钮功能正常，注意到一个细节：A（拨轮开关）按压触发；BC释放触发。尽管程序写的是setPressedHandler。暂无大碍。

2023.1.22 尝试删掉抄来的GUI...
-23:41 截至目前，没有发现严重问题，此版通过，命名为1.0

2023.1.23
-0:54 完成GUI结合，存在长按返回触发键盘输出的Bug，暂无头绪。

2023.1.28 TestBuild01a
尝试添加title选中点击后的二级菜单。
失败

2023.2.12 TestBuild01b
-0：21 尝试添加演示代码。

2023.2.19 
修正Demo
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
#define EVENT_SINGLE_CLICK 1 // 单击旋转编码器
#define EVENT_LONG_CLICK 2   // 按住旋转编码器上的按钮1秒钟以上（？）（直接从库中的默认值更改）
#define EVENT_ROTATE_CW 3    // 顺时针旋转旋转编码器
#define EVENT_ROTATE_CC 4    // 逆时针旋转旋转编码器
#define EVENT_ROTATE_NONE 5

#define UI_HOME 0     // ui_home控制的主界面
#define UI_PASSWD 1   // 展示密码标题
#define UI_KEYBOARD 2 // 键盘模式
#define UI_SETTING 3  // 设置
#define UI_DEMOBLOWFISH 4 // 河豚演示
#define UI_DEMOSHUTDOWN 5
#define UI_DEMOCANCEL 6

#define LONGCLICK_MS 800

Button2 Button_A = Button2(BUTTON_PIN_A);  
Button2 Button_B = Button2(BUTTON_PIN_B);  
Button2 Button_C = Button2(BUTTON_PIN_C);  

Rotary r = Rotary(9, 8);

I2C_eeprom eeprom_a(EEPROM_A_ADDR, I2C_DEVICESIZE_24LC256); // 0x50->A  0x51->B
I2C_eeprom eeprom_b(EEPROM_B_ADDR, I2C_DEVICESIZE_24LC256); // 0x50->A  0x51->B

I2C_eeprom ee = eeprom_a; // 使用A槽位

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
// U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
// 空间不足，先用U8X8顶一下


/******************相关变量*******************/
char VERSION = "1.1 Build1a";

char title[30];
char user[60];
char data[60];


int currentIndex = 0;
int lastIndex = 0; 

int dataNum = 0;


typedef struct
{
  char* str;
  byte len;
}SETTING_LIST;

SETTING_LIST list[] = 
{
  {"Password",8},
  {"Keyboard",8},
  {"Settings",8},
  {"DemoBlowFish",12},
  {"DemoShutdown",12},
  {"DemoCancel",10},
};

short x,x_trg; //x当前位置数值,x_trg 目标坐标值
short y = 15,y_trg = 15;

short frame_len,frame_len_trg;//框框的宽度
short frame_y,frame_y_trg;//框框的y

char ui_select = 0;
char ui_last_select = 0;
char ui_top_sn = 0;

char methods_num = 0;
char select_method = 0;

int state = UI_HOME;
int last_state = state; // 用于长按返回上一界面

volatile uint8_t event = EVENT_NONE;
volatile uint8_t flag = 0;
/*******************************************/

// 拨轮中断
ISR(PCINT0_vect) {
  unsigned char result = r.process();
  int flagIndex = currentIndex;
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
  }else{
    // event = EVENT_ROTATE_NONE;
  }if (currentIndex < 0){
    // currentIndex = 0;
  }
  Serial.print("currentIndex: ");
  Serial.println(currentIndex);
  // if(currentIndex != flagIndex) {
  //   DataProcess(currentIndex);
  // }
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

/*
char titlelist[5][30];  // 统一标准，0不使用
void TitlePreLoad() {
  ConfReading();
  for(int i = 1; i <= dataNum; i++) {
    char buff;
    for (int j = 0; j <= 30; j++) {
      buff = ee.readByte(150*i+i);
      titlelist[i][j] = buff;
      Serial.print(buff);    
      if(buff == '\0'){
        break;
      }
    }
  }
}

void TitlePreLoadTest() {
  for(int i = 1; i <= dataNum; i++) {
    Serial.println(titlelist[i]);
  }
}
*/

///////////////////UI部分////////////////////////

int ui_run(short *a,short *a_trg,u8 step,u8 slow_cnt)
{  
  u8 temp;

  temp = abs(*a_trg-*a) > slow_cnt ? step : 1;

  if(*a < *a_trg)
  {
    *a += temp;
  }
  else if( *a > *a_trg)
  {
    *a -= temp;  
  }
  else
  {
    return 0;
  }
  return 1;
    *a = *a_trg;
}

void ui_show(void)
{
  int list_len = sizeof(list) / sizeof(SETTING_LIST);
  u8g2.clearBuffer();         // 清除内部缓冲区
  for(int i = 0, j = 0 ;i < list_len;i++)
  {
    u8g2.drawStr(x+2,y+(i-ui_top_sn)*16,list[i].str);
  }
  u8g2.setDrawColor(2);
  u8g2.drawRBox(x,frame_y,frame_len,16,3);
  u8g2.setDrawColor(1);
  ui_run(&frame_y,&frame_y_trg,5,4);
  ui_run(&frame_len,&frame_len_trg,10,5);
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void ui_home(void)
{
  int list_len = sizeof(list) / sizeof(SETTING_LIST);
  if(event != EVENT_ROTATE_NONE) {
    if(event == EVENT_ROTATE_CW) {
      ui_select++;
      if(ui_select >= list_len) {
        ui_select = list_len - 1;  
      }
    }else if(event == EVENT_ROTATE_CC) {
      ui_select--;
      if(ui_select < 0) {
          ui_select = 0;  
      }
    }else {
      Serial.println("It's impossible!");
    }
    event = EVENT_ROTATE_NONE;
    if ((ui_select) % 2 == 0) {
      ui_top_sn = ui_select;
    }
    if (ui_select < ui_top_sn) {
      ui_top_sn -= 2 ;
    }    

    frame_y_trg = (ui_select) % 2 * 16;
    frame_len_trg = list[ui_select].len*10;
    ui_last_select = ui_select;
    Serial.print("ui_select: ");
    Serial.println(ui_select);
  }

  ui_show();
}



int selectedIndex = 1;  // 1是第一位，首个分区（150 Bytes）被配置数据占了。
int lastSelectedIndex = 1;
void ui_passwd(){
  // currentIndex = 0;
  u8g2.clearBuffer();         // 清除内部缓冲区
  // u8g2.drawStr(0,10,"title");

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

void ui_keyboard(){
  u8g2.clearBuffer();         // 清除内部缓冲区
  u8g2.drawStr(0,12,"Keyboard Mode");
  u8g2.drawStr(0,30,"Developing...");
  u8g2.sendBuffer();
}

void ui_setting() {
  u8g2.clearBuffer();         // 清除内部缓冲区
  u8g2.drawStr(0,12,"V1.1 Build 1b");
  u8g2.drawStr(0,30,"Lsnig@115200");
  u8g2.sendBuffer();
  debugProc();
}

void ui_demoBlowFish(){
  u8g2.clearBuffer();         // 清除内部缓冲区
  u8g2.drawStr(0,12,"DemoBlowFish");
  u8g2.drawStr(0,30,"...");
  u8g2.sendBuffer();
  debugProc();
}

void ui_demoShutdown(){
  u8g2.clearBuffer();         // 清除内部缓冲区
  u8g2.drawStr(0,12,"Shutdown");
  u8g2.drawStr(0,30,"...");
  u8g2.sendBuffer();
  debugProc();
}

void ui_demoCancel(){
  u8g2.clearBuffer();         // 清除内部缓冲区
  u8g2.drawStr(0,12,"Cancel");
  u8g2.drawStr(0,30,"...");
  u8g2.sendBuffer();
  debugProc();
}

/// Demo Part

void demoPrint(char chr){
  Keyboard.press(chr);
  Keyboard.release(chr);
  delay(200);
}

void demoBlowFish(){


  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  delay(100);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release('r');

  Keyboard.print("notepad");
  delay(100);
  Keyboard.press(KEY_RETURN);
  Keyboard.release(KEY_RETURN);
  delay(500);

  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_SHIFT);
  delay(300);
  Keyboard.release(KEY_LEFT_SHIFT);
  Keyboard.release(KEY_LEFT_CTRL);

  Keyboard.press(KEY_CAPS_LOCK);
  Keyboard.release(KEY_CAPS_LOCK);

  delay(500);
  demoPrint('y');
  demoPrint('o');
  demoPrint('u');
  demoPrint(' ');
  demoPrint('a');
  demoPrint('r');
  demoPrint('e');
  demoPrint(' ');
  demoPrint('n');
  demoPrint('o');
  demoPrint('t');
  demoPrint(' ');
  demoPrint('a');
  demoPrint(' ');
  demoPrint('b');
  demoPrint('l');
  demoPrint('o');
  demoPrint('w');
  demoPrint('f');
  demoPrint('i');
  demoPrint('s');
  demoPrint('h');
  demoPrint('!');
  //Keyboard.print("YOU ARE NOT A BLOWFISH!");

  Keyboard.press(KEY_CAPS_LOCK);
  Keyboard.release(KEY_CAPS_LOCK);
}

void demoBlowFishPic(){
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  delay(100);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release('r');

  Keyboard.print("c://BKB_DEMO/demo.jpg");
  delay(100);
  Keyboard.press(KEY_RETURN);
  Keyboard.release(KEY_RETURN);
  delay(500);
}

void demoShutdown(){
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_SHIFT);
  delay(300);
  Keyboard.release(KEY_LEFT_SHIFT);
  Keyboard.release(KEY_LEFT_CTRL);

  delay(500);

  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  delay(100);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release('r');

  Keyboard.print("SHUTDOWN -S -T 300");
  delay(100);

  Keyboard.press(KEY_RETURN);
  Keyboard.release(KEY_RETURN);

}

void demoCancel(){
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_SHIFT);
  delay(300);
  Keyboard.release(KEY_LEFT_SHIFT);
  Keyboard.release(KEY_LEFT_CTRL);

  delay(500);

  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  delay(100);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release('r');

  Keyboard.print("SHUTDOWN -A");
  delay(100);

  Keyboard.press(KEY_RETURN);
  Keyboard.release(KEY_RETURN);
}

///

void ui_proc() {
  switch(state) {
    case UI_HOME:
      ui_home();
      break;
    case UI_PASSWD:
      ui_passwd();
      break;
    case UI_KEYBOARD:
      ui_keyboard();
      break;
    case UI_SETTING:
      ui_setting();
      break;
    case UI_DEMOBLOWFISH:
      ui_demoBlowFish();
      break;
    case UI_DEMOSHUTDOWN:
      ui_demoShutdown();
      break;
    case UI_DEMOCANCEL:
      ui_demoCancel();
      break;
    default:
      break;
  }
}

///////按键回调函数///////
bool switchFlag = 0;
void ButtonA() {
  Serial.println("A");
  switch(state) {
    case UI_HOME:
      switch(ui_select) {
        case 0:
          state = UI_PASSWD;
          ConfReading();
          break;
        case 1:
          state = UI_KEYBOARD;
          break;
        case 2:
          state = UI_SETTING;
          break;  
        case 3:
          state = UI_DEMOBLOWFISH;
          break; 
        case 4:
          state = UI_DEMOSHUTDOWN;
          break; 
        case 5:
          state = UI_DEMOCANCEL;
          break;      
      }
      switchFlag = 1;
    case UI_PASSWD:
      if(switchFlag){
        switchFlag = 0;
        break;
      }
      else{
        Serial.println("A");
        UserLoading(selectedIndex);
        DataLoading(selectedIndex);

        Serial.println(user);
        Keyboard.print(user);

        Keyboard.press(KEY_TAB);
        Keyboard.release(KEY_TAB);

        Serial.println(data);
        Keyboard.print(data);
        break;
      }
    case UI_SETTING:
      break;
    case UI_DEMOBLOWFISH:
      demoBlowFish();
      break;
    case UI_DEMOSHUTDOWN:
      demoShutdown();
      break;
    case UI_DEMOCANCEL:
      demoCancel();
      break;
  }
}

/*
void ButtonA_longClickDetected (Button2& btn){
  switchFlag = 1;
  state = UI_HOME;
  Serial.print("long click #");
  Serial.print(btn.getLongClickCount());
  Serial.println(" detected");
}
*/

void ButtonA_LongClickHandler(Button2& btn) {
  if (btn.wasPressedFor() > LONGCLICK_MS){
    switchFlag = 1;
    state = UI_HOME;
    Serial.print("long click");
  }
}

// BC按键有Bug，应该是硬件（上下拉）错误。上电会触发一次。

int BFirstTap = 0;
void ButtonB() {
  Serial.println("B");
  Serial.println(BFirstTap);
  BFirstTap++;
  // UserLoading(selectedIndex);
  // Serial.println(user);
  // Keyboard.print(user);
  switch(state) {
    case UI_HOME:
      break;
    case UI_PASSWD:
      if(switchFlag){
        switchFlag = 0;
        break;
      }
      else{
        UserLoading(selectedIndex);
        Serial.println(user);
        Keyboard.print(user);
        break;
      }
    case UI_SETTING:
      break;
    case UI_DEMOBLOWFISH:
      demoBlowFish();
      break;
    case UI_DEMOSHUTDOWN:
      demoShutdown();
      break;
    case UI_DEMOCANCEL:
      demoCancel();
      break;
  }
}

int AFirstTap = 0;
void ButtonC() {
  Serial.println("C");
  // DataLoading(selectedIndex);
  // Serial.println(data);
  // Keyboard.print(data);
  switch(state) {
    case UI_HOME:
      break;
    case UI_PASSWD:
      if(switchFlag){
        switchFlag = 0;
        break;
      }
      else{
        DataLoading(selectedIndex);
        Serial.println(data);
        Keyboard.print(data);
        break;
      }
    case UI_SETTING:
      break;
    case UI_DEMOBLOWFISH:
      demoBlowFishPic();
      break;
    case UI_DEMOSHUTDOWN:
      demoShutdown();
      break;
    case UI_DEMOCANCEL:
      demoCancel();
      break;
  }
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

  Button_A.setClickHandler(ButtonA);
  // Button_A.setLongClickDetectedHandler(ButtonA_longClickDetected);
  Button_A.setLongClickHandler(ButtonA_LongClickHandler);
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
  ui_proc();
  Button_A.loop();
  Button_B.loop();
  Button_C.loop();
  // debugProc(); // 放到SETTING里去

}
