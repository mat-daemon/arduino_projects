#include <LiquidCrystal_I2C.h>
#include <util/atomic.h>

#define LED_RED 6
#define LED_GREEN 5
#define LED_BLUE 3

#define ENCODER1 A2
#define ENCODER2 A3

#define RED_BUTTON 2
#define GREEN_BUTTON 4

#define DEBOUNCING_PERIOD 100
#define BUTTON_DEBOUNCE_PERIOD 10UL

#define UP 0
#define DOWN 1

#define ENC_MENU_STATE 0
#define ENC_LED_STATE 1
#define ENC_BRIGHT_STATE 2

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte selection_dot[8] = {
  B00000,
  B00000,
  B01110,
  B01110,
  B01110,
  B00000,
  B00000,
  B00000,
};

int brightness = 255;
const int menu_length = 4;
String menu[menu_length] = {"Turn LED ON     ", "Turn LED OFF    ", "LED: RED        ", "Brightness: 255"};
int menu_index = 0;
int current_lcd_row = 0;

void init_LCD(){
  lcd.print(menu[0]);
  lcd.setCursor(0, 1);
  lcd.print(menu[1]);
  lcd.createChar(0, selection_dot);
  lcd.setCursor(15, 0);
  lcd.write(byte(0));
}


void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  pinMode(GREEN_BUTTON, INPUT_PULLUP);
  pinMode(RED_BUTTON, INPUT_PULLUP);

  pinMode(ENCODER1, INPUT_PULLUP);
  pinMode(ENCODER2, INPUT_PULLUP);

  PCICR |=  (1 << PCIE1);
  PCMSK1 |= (1 << PCINT10);

  lcd.init();
  lcd.backlight();
  init_LCD();
}


int enc_state = ENC_MENU_STATE;
volatile int encoder1 = HIGH;
volatile int encoder2 = HIGH;
volatile unsigned long encoderTimeStamp = 0UL;
ISR(PCINT1_vect){
  encoder1 = digitalRead(ENCODER1);
  encoder2 = digitalRead(ENCODER2);
  encoderTimeStamp = millis();
}


int LEDS[3] = {LED_RED, LED_GREEN, LED_BLUE};
int LED_index = 0;

void moveMenu(int direction){
  //delete current selection dot
  lcd.setCursor(15, current_lcd_row);
  lcd.print(" ");

  if(direction == DOWN){
    menu_index = (menu_index+1)%menu_length; 

    if(current_lcd_row == 0){
      current_lcd_row = 1;
    }
    else{
      if(menu_index == 0){
        current_lcd_row = 0;
        lcd.setCursor(0, 0);
        lcd.print(menu[menu_index]);
        lcd.setCursor(0, 1);
        lcd.print(menu[menu_index+1]);   
      } 
      else{
        lcd.setCursor(0, 0);
        lcd.print(menu[menu_index-1]);
        lcd.setCursor(0, 1);
        lcd.print(menu[menu_index]);   
      }
    }
  }
  else{
    menu_index = (menu_index-1) < 0 ? menu_length-1 : menu_index-1; 

    if(current_lcd_row == 1){
      current_lcd_row = 0;
    }
    else{
      if(menu_index == menu_length-1){
        current_lcd_row = 1;
        lcd.setCursor(0, 0);
        lcd.print(menu[menu_index-1]);
        lcd.setCursor(0, 1);
        lcd.print(menu[menu_index]);   
      } 
      else{
        lcd.setCursor(0, 0);
        lcd.print(menu[menu_index]);
        lcd.setCursor(0, 1);
        lcd.print(menu[menu_index+1]);   
      }
    }
  }
  lcd.setCursor(15, current_lcd_row);
  lcd.write(byte(0));
}


void performAction(){
    
  switch(menu_index){
    case 0:
      analogWrite(LEDS[LED_index], brightness);        
      break;
    case 1:
      analogWrite(LEDS[LED_index], 0);
      break;
    case 2:
      enc_state = enc_state == 1 ? 0 : 1;   
      break;
    case 3:
      enc_state = enc_state == 2 ? 0 : 2; 
      break;
  }

}

void changeLED(int direction){ 
   
  if(direction == UP) LED_index = (LED_index+1)%3;
  else LED_index = LED_index-1 < 0 ? 2 : (LED_index-1);
    
  switch(LEDS[LED_index]){
    case LED_RED:
      menu[2] = "LED: RED        ";
      break;
    case LED_GREEN:
      menu[2] = "LED: GREEN      ";
      break;
    case LED_BLUE:
      menu[2] = "LED: BLUE       ";
      break;
  }

  lcd.setCursor(0, current_lcd_row);
  lcd.print(menu[2]);
  lcd.setCursor(15, current_lcd_row);
  lcd.write(byte(0));
}


void changeBrightness(int direction){
  if(direction == UP) brightness = min(255, brightness+15);
  else brightness = max(0, brightness-15);

  String white_space = "";
  int ws_length = 16 - 12 - String(brightness).length();
  for(int i=0; i<ws_length; i++) white_space += " ";

  menu[3] = "Brightness: " + String(brightness) + white_space;
  lcd.setCursor(0, current_lcd_row);
  lcd.print(menu[3]);
  lcd.setCursor(15, current_lcd_row);
  lcd.write(byte(0));
  analogWrite(LEDS[LED_index], brightness);
}

bool isGreenButtonPressed(){
  static int debounced_button_state = HIGH;
  static int previous_reading = HIGH;
  static unsigned long last_change_time = 0UL;
  bool isPressed = false;

  int current_reading = digitalRead(GREEN_BUTTON);

  if (previous_reading != current_reading) last_change_time = millis();

  if (millis() - last_change_time > BUTTON_DEBOUNCE_PERIOD){
    if (current_reading != debounced_button_state){
      if (debounced_button_state == HIGH && current_reading == LOW){
        isPressed = true;
      }
      debounced_button_state = current_reading;
    }
  }
  previous_reading = current_reading;

  return isPressed;
}


bool isRedButtonPressed(){
  static int debounced_button_state = HIGH;
  static int previous_reading = HIGH;
  static unsigned long last_change_time = 0UL;
  bool isPressed = false;

  int current_reading = digitalRead(RED_BUTTON);

  if (previous_reading != current_reading) last_change_time = millis();

  if (millis() - last_change_time > BUTTON_DEBOUNCE_PERIOD){
    if (current_reading != debounced_button_state){
      if (debounced_button_state == HIGH && current_reading == LOW){
        isPressed = true;
      }
      debounced_button_state = current_reading;
    }
  }
  previous_reading = current_reading;

  return isPressed;
}


int encoderValue = 0;
int lastEn1 = LOW;
unsigned long lastChangeTimeStamp = 0UL;


void loop() {
  int en1;
  int en2;
  unsigned long timestamp;
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    en1 = encoder1;
    en2 = encoder2;
  }


  if(en1 == LOW && millis() > lastChangeTimeStamp + DEBOUNCING_PERIOD){
    if(enc_state == ENC_MENU_STATE){
      if(en2 == HIGH) moveMenu(DOWN);
      else moveMenu(UP);
    }
    else if(enc_state == ENC_LED_STATE){
      if(en2 == HIGH) changeLED(UP);
      else changeLED(DOWN);
    }
    else if(enc_state == ENC_BRIGHT_STATE){
      if(en2 == HIGH) changeBrightness(UP);
      else changeBrightness(DOWN);        
    }
    lastChangeTimeStamp = millis();
  }
  
  if(isGreenButtonPressed()){
    performAction();
  }

}
