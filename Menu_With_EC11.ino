
#include <Rotary.h>
#include "Button2.h"

#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>

#include <SPI.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#define BUTTON_PIN  5


#define EVENT_NONE 0         // 用于切换上一个事件以避免无限循环
#define EVENT_SINGLE_CLICK 1 // 单击旋转编码器
#define EVENT_LONG_CLICK 2   // 按住旋转编码器上的按钮1秒钟以上（？）（直接从库中的默认值更改）
#define EVENT_ROTATE_CW 3    // 顺时针旋转旋转编码器
#define EVENT_ROTATE_CC 4    // 逆时针旋转旋转编码器
#define EVENT_ROTATE_NONE 5

#define MENU_HOME 0
#define MENU_METHODS 1


Rotary r = Rotary(9, 8);

Button2 encoderButton = Button2(BUTTON_PIN);  


File myFile;

volatile uint8_t event = EVENT_NONE;
volatile uint8_t flag = 0;

volatile uint8_t current_menu = MENU_HOME;

typedef struct
{
  byte val;
  byte last_val;
}KEY_T;
typedef struct
{
  byte direct;
  byte isnone;
  byte update_flag;
  byte res;
}KEY_MSG;

typedef struct
{
  char* str;
  byte len;
}SETTING_LIST;

SETTING_LIST list[] = 
{
  {"Methods",7},
  {"Favorites",9},
  {"Plugins",7},
  {"Settings",8},
};

/*
typedef struct
{
  char* title;
  byte title_len;
}METHODS_LIST;

METHODS_LIST methods[];
*/
char method_title;
char method_title_len;

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

short x,x_trg; //x当前位置数值,x_trg 目标坐标值
short y = 15,y_trg = 15;

short frame_len,frame_len_trg;//框框的宽度
short frame_y,frame_y_trg;//框框的y

char ui_select = 0;
char ui_last_select = 0;
char ui_top_sn = 0;

char methods_num = 0;
char select_method = 0;

int state;
KEY_MSG key_msg = {0};


ISR(PCINT0_vect) {
  unsigned char result = r.process();
  Serial.println(result);
  if (result != DIR_NONE) {
    if (result == DIR_CW) {
      event = EVENT_ROTATE_CW;
    } else if (result == DIR_CCW) {
      event = EVENT_ROTATE_CC;
    }
  }else{
    // event = EVENT_ROTATE_NONE;
  }
  
  
}

void sd_init() {
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
}
ReadBufferingStream bufferingStream(myFile, 64);


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

void setup(void) 
{
  Serial.begin(9600);
  r.begin();
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT5) | (1 << PCINT4);
  sei();

  encoderButton.setReleasedHandler(buttonReleasedHandler); //编码器按钮
  encoderButton.setDoubleClickHandler(doubleClickHandler);

  // SD.begin();

  u8g2.begin();
  u8g2.setFont(u8g2_font_t0_18_mf ); //设置字体
  // frame_len = frame_len_trg = list[ui_select].len*10;
  // sd_init();
}

void buttonReleasedHandler(Button2 &btn) {
  Serial.println("buttonReleasedHandler");
}
void doubleClickHandler(Button2 &btn) {
  Serial.println("doubleClickHandler");
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

void ui_proc(void)
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
  }

  ui_show();
}

void loop(void) 
{
  ui_proc();  
  encoderButton.loop();
}