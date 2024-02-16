
#include <Arduino.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <Versatile_RotaryEncoder.h>
#include <Adafruit_INA219.h>
// Set here your encoder reading pins
Adafruit_INA219 ina219;
#define clk 2
#define dt 3
#define sw 4
#define pwm 9
// Create encoder object
Versatile_RotaryEncoder versatile_encoder(clk, dt, sw);

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

MUIU8G2 mui;

/*
  global variables which form the communication gateway between the user interface and the rest of the code
*/

uint8_t num_value = 0;
uint8_t bar_value = 0;
uint16_t animal_idx = 0;
uint8_t i_power = 0;
uint8_t m_power = 0;
uint8_t i_current = 0;
uint8_t m_current = 0;
uint8_t ms = 0;
float voltage = 0;
float set_current = 0;
float current = 0;
float power = 0;
float set_power = 0;
float current_last = 0;
float voltage_last = 0;
volatile boolean currentmode = false;
volatile boolean powermode = false;
float value = 176.875;
uint8_t mui_hrule(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
      u8g2.drawHLine(0, mui_get_y(ui), u8g2.getDisplayWidth());
  }
  return 0;
}

uint8_t show_voltage(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);
    u8g2.setCursor(x+50, y);
    u8g2.print(voltage);
    }
  return 0;
}
uint8_t show_current(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);
    u8g2.setCursor(x+50, y+12);
    u8g2.print(current);
    }
  return 0;
}
uint8_t show_dot(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);
    u8g2.setCursor(x, y);
    u8g2.print(".");
    }
  return 0;
}
uint8_t show_power(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);
    u8g2.setCursor(x, y);
    u8g2.print(power);
    }
  return 0;
}
uint8_t start_power(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    powermode = true;
    }
  return 0;
}
uint8_t stop_power(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    powermode = false;
    currentmode = false;
    OCR1A = 0;
    }
  return 0;
}
uint8_t start_current(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    currentmode = true;
    }
  return 0;
}
uint8_t stop_current(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    currentmode = false;
    powermode = false;
    OCR1A = 0;
    }
  return 0;
}
muif_t muif_list[] = {
  MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),        /* regular font */
  MUIF_U8G2_FONT_STYLE(1, u8g2_font_helvB08_tr),        /* bold font */

  MUIF_RO("HR", mui_hrule),
  MUIF_U8G2_LABEL(),
  MUIF_RO("GP",mui_u8g2_goto_data),  
  MUIF_BUTTON("GC", mui_u8g2_goto_form_w1_pi),

  MUIF_U8G2_U8_MIN_MAX("NV", &i_power, 0, 32, mui_u8g2_u8_min_max_wm_mud_pi),
  MUIF_U8G2_U8_MIN_MAX("NX", &m_power, 0, 9, mui_u8g2_u8_min_max_wm_mud_pi),
  MUIF_U8G2_U8_MIN_MAX("IV", &i_current, 0, 3, mui_u8g2_u8_min_max_wm_mud_pi),
  MUIF_U8G2_U8_MIN_MAX("IX", &m_current, 0, 9, mui_u8g2_u8_min_max_wm_mud_pi),
  MUIF_U8G2_U8_MIN_MAX_STEP("NB", &bar_value, 0, 16, 1, MUI_MMS_2X_BAR|MUI_MMS_SHOW_VALUE, mui_u8g2_u8_bar_wm_mud_pf),//this one
  MUIF_U8G2_U8_MIN_MAX_STEP("NT", &bar_value, 0, 16, 1, MUI_MMS_2X_BAR|MUI_MMS_SHOW_VALUE, mui_u8g2_u8_bar_wm_mud_pf),//uult

  /* register custom function to show the data */
 
  MUIF_RO("AV", show_voltage),
  MUIF_RO("AC", show_current),
  MUIF_RO("AP", show_power),
  MUIF_RO("AD", show_dot),
  MUIF_RO("ST", start_power),
  MUIF_RO("SP", stop_power),
  MUIF_RO("SX", start_current),
  MUIF_RO("SY", stop_current),
  /* a button for the menu... */
  //MUIF_BUTTON("GO", mui_u8g2_btn_goto_wm_fi)  
  MUIF_EXECUTE_ON_SELECT_BUTTON("GO", mui_u8g2_btn_goto_wm_fi)  
  
  
};


fds_t fds_data[] = 

MUI_FORM(1)
MUI_AUX("SP")
MUI_AUX("SY")
MUI_STYLE(1)
MUI_LABEL(6, 8, "Programmable DC Load")
MUI_STYLE(0)
MUI_XY("HR", 0,11)
MUI_DATA("GP", 
    MUI_10 "Power Mode|"
    MUI_12 "Current Mode|"
    MUI_14 "Multi-Meter Mode")
MUI_XYA("GC", 5, 30, 0) 
MUI_XYA("GC", 5, 45, 1) 
MUI_XYA("GC", 5, 60, 2) 

MUI_FORM(10)
MUI_AUX("SP")
MUI_STYLE(1)
MUI_LABEL(34, 8, "Power Mode")
MUI_XY("HR", 0,11)
MUI_STYLE(0)
MUI_LABEL(5,23, "Actual Voltage:")
MUI_LABEL(5,36, "Actual Current:")
MUI_LABEL(5,49, "Set Power:")
MUI_STYLE(1)
MUI_LABEL(102,23, "V")
MUI_LABEL(102,36, "A")
MUI_LABEL(83,49, "W")
MUI_XY("AV", 30, 23)
MUI_XY("AC", 30, 24)
MUI_XY("NV", 60, 49)
MUI_XY("NX", 75, 49)
MUI_XY("AD", 72, 49)
MUI_STYLE(0)
//MUI_XYA("NA", 50, 49, 44)
MUI_XYAT("GO", 110, 60, 1, " Back ") 
MUI_XYAT("GO", 20, 60, 11, " Start ")

MUI_FORM(11)
MUI_AUX("ST")
MUI_STYLE(1)
MUI_LABEL(34, 8, "Power Mode")
MUI_XY("HR", 0,11)
MUI_STYLE(0)
MUI_LABEL(5,23, "Actual Voltage:")
MUI_LABEL(5,36, "Actual Current:")
MUI_LABEL(5,49, "Set Power:")
MUI_STYLE(1)
MUI_LABEL(102,23, "V")
MUI_LABEL(102,36, "A")
MUI_LABEL(83,49, "W")
MUI_XY("AV", 30, 23)
MUI_XY("AC", 30, 24)
MUI_XY("NV", 60, 49)
MUI_XY("NX", 75, 49)
MUI_XY("AD", 72, 49)
MUI_STYLE(0)
//MUI_XYA("NA", 50, 49, 44)
MUI_XYAT("GO", 110, 60, 1, " Back ") 
MUI_XYAT("GO", 20, 60, 10, " Stop ")

MUI_FORM(12)
MUI_AUX("SY")
MUI_STYLE(1)
MUI_LABEL(30, 8, "Current Mode")
MUI_XY("HR", 0,11)
MUI_STYLE(0)
MUI_LABEL(5,23, "Actual Voltage:")
MUI_LABEL(5,36, "Actual Current:")
MUI_LABEL(5,49, "Set Current:")
MUI_STYLE(1)
MUI_LABEL(102,23, "V")
MUI_LABEL(102,36, "A")
MUI_LABEL(85,49, "A")
MUI_XY("AV", 30, 23)
MUI_XY("AC", 30, 24)
MUI_XY("IV", 68, 49)
MUI_XY("IX", 77, 49)
MUI_XY("AD", 74, 49)
MUI_STYLE(0)
MUI_XYAT("GO", 110, 60, 1, " Back ") 
MUI_XYAT("GO", 20, 60, 13, " Start ")

MUI_FORM(13)
MUI_AUX("SX")
MUI_STYLE(1)
MUI_LABEL(30, 8, "Current Mode")
MUI_XY("HR", 0,11)
MUI_STYLE(0)
MUI_LABEL(5,23, "Actual Voltage:")
MUI_LABEL(5,36, "Actual Current:")
MUI_LABEL(5,49, "Set Current:")
MUI_STYLE(1)
MUI_LABEL(102,23, "V")
MUI_LABEL(102,36, "A")
MUI_LABEL(85,49, "A")
MUI_XY("AV", 30, 23)
MUI_XY("AC", 30, 24)
MUI_XY("IV", 68, 49)
MUI_XY("IX", 77, 49)
MUI_XY("AD", 74, 49)
MUI_STYLE(0)
MUI_XYAT("GO", 110, 60, 1, " Back ") 
MUI_XYAT("GO", 20, 60, 12, " Stop ")


MUI_FORM(14)
MUI_AUX("SP")
MUI_AUX("SY")
MUI_STYLE(1)
MUI_LABEL(20, 8, "Multi-Meter Mode")
MUI_XY("HR", 0,11)
MUI_STYLE(0)
MUI_LABEL(5,23, "Actual Voltage:")
MUI_LABEL(5,36, "Actual Current:")
MUI_LABEL(5,49, "Actual Power:")
MUI_XY("AV", 30, 23)
MUI_XY("AC", 30, 24)
MUI_XY("AP", 74, 49)

MUI_XYAT("GO", 110, 60, 1, " Back ") 

;

// global variables for menu redraw and input event handling
uint8_t is_redraw = 1;
uint8_t rotate_event = 0; // 0 = not turning, 1 = CW, 2 = CCW
uint8_t press_event = 0; // 0 = not pushed, 1 = pushed
uint8_t long_press_event = 0; // 0 = not pushed, 1 = pushed


// Functions prototyping to be handled on each Encoder Event
void handleRotate(int8_t rotation) {
  if ( rotation > 0 )
    rotate_event = 2; // CW
  else
    rotate_event = 1; // CCW
}

void handlePressRelease() {
  press_event = 1;
}

void handleLongPressRelease() {
  long_press_event = 1;
}

ISR(TIMER2_OVF_vect)                   
{
  TCNT2 = value;
  ms++;
  
  if(ms >= 150){
    
    if ( mui.getCurrentFormId() == 11)
    {
    is_redraw = 1;
    }
    if ( mui.getCurrentFormId() == 13)
    {
    is_redraw = 1;
    }
    if ( mui.getCurrentFormId() == 14)
    {
    is_redraw = 1;
    }
    ms = 0;
  }
  
  
  }


void setup(void) {
  //Serial.begin(115200);
  while(!ina219.begin()){
    //Serial.println("ina");
    delay(100);
    }
  pinMode(pwm, OUTPUT);
  digitalWrite(pwm, LOW);
  TCCR1A = 0;
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  TCCR1B = 0;
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
  ICR1 = 2047;
  OCR1A = 0;
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = value;                        // preload timer
  TCCR2B |= (1<<CS22) | (1 << CS21)|(1 << CS20);    // 1024 prescaler 
  TIMSK2 |= (1 << TOIE2);               // enable timer overflow interrupt ISR
  // Load to the encoder all nedded handle functions here (up to 9 functions)
  versatile_encoder.setHandleRotate(handleRotate);
  versatile_encoder.setHandlePressRelease(handlePressRelease);
  versatile_encoder.setHandleLongPressRelease(handleLongPressRelease);
  
  u8g2.begin();
  mui.begin(u8g2, fds_data, muif_list, sizeof(muif_list)/sizeof(muif_t));
  mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
}


void handle_events(void) {
  // 0 = not pushed, 1 = pushed  
  if ( press_event == 1 ) {
    mui.sendSelect();
    is_redraw = 1;
    press_event = 0;
  }

  // 0 = not pushed, 1 = pushed  
  if ( long_press_event == 1 ) {
    mui.sendSelectWithExecuteOnSelectFieldSearch();
    is_redraw = 1;
    long_press_event = 0;
  }
  
  // 0 = not turning, 1 = CW, 2 = CCW
  if ( rotate_event == 1 ) {
    mui.nextField();
    is_redraw = 1;
    rotate_event = 0;
  }
  
  if ( rotate_event == 2 ) {
    mui.prevField();
    is_redraw = 1;
    rotate_event = 0;
  }    
}



void loop(void) {

  set_power = i_power + (m_power*0.1);
  set_current = i_current + (m_current*0.1);
  //Serial.println(set_current);

  current = ina219.getCurrent_mA()/1000;
  power = ina219.getPower_mW()/1000;
  voltage = ina219.getBusVoltage_V() + (ina219.getShuntVoltage_mV()/1000);

    if (currentmode) {
    
    if (current < set_current) {
      OCR1A++;
    }
    else {
      OCR1A = OCR1A - 1;
    }
    //Serial.println(OCR1A);
    }


    if (powermode) {

    if (power < set_power) {
      OCR1A++;
    }
    else {
      OCR1A = OCR1A - 1;
    }

  }

 
  /* check whether the menu is active */
  if ( mui.isFormActive() ) {

    /* update the display content, if the redraw flag is set */
    if ( is_redraw ) {
      u8g2.firstPage();
      do {
          versatile_encoder.ReadEncoder(); // Do the encoder reading and processing
          mui.draw();
          versatile_encoder.ReadEncoder(); // Do the encoder reading and processing
      } while( u8g2.nextPage() );
      is_redraw = 0;                    /* clear the redraw flag */
    }

    
    versatile_encoder.ReadEncoder(); // Do the encoder reading and processing
    handle_events();
      
  } else {
      /* the menu should never become inactive, but if so, then restart the menu system */
      mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
  }
  voltage_last = voltage;
  current_last = current;
}


