#include "GuiManager.h"

#include "Marlin.h"
#include "cardreader.h"
#include "ConfigurationStore.h"
#include "temperature.h"
#include "language.h"

#include "ScreenMenu.h"
#include "ScreenDialog.h"
#include "ScreenSelector.h"
#include "ScreenList.h"
#include "DOGMbitmaps.h"

/////////////////////////////////////////////////////////////////////////
//                          Marlin interface                           //
/////////////////////////////////////////////////////////////////////////
void draw_status_screen(){};
void draw_wizard_change_filament(){};



/////////////////////////////////////////////////////////////////////////
//                     Build screens and menu structure                //
/////////////////////////////////////////////////////////////////////////
screen::Icon icon_sd_normal = screen::Icon(24, 22, sd_normal_bits);
screen::Icon icon_sd_hover = screen::Icon(24, 22, sd_hover_bits);

//Logo Splasg screen
screen::ScreenDialog screen_logo = screen::ScreenDialog("BQ Logo");
//Main Menu screen
screen::ScreenMenu screen_main = screen::ScreenMenu("Main Menu");
//SD Card screens
screen::ScreenList screen_SD_list = screen::ScreenList("SD Card"); 
screen::ScreenMenu screen_SD_confirm = screen::ScreenMenu("Comfirm Print");
screen::ScreenList screen_SD_back = screen::ScreenList("Back");
screen::ScreenMenu screen_SD_continue = screen::ScreenMenu("OK");
//screen::ScreenMenu screen_main_printing = screen::ScreenMenu("Printing Menu"); 
//Unload Filament screens
screen::ScreenSelector screen_unload_select = screen::ScreenSelector("Mount filament");
screen::ScreenDialog screen_unload_heating = screen::ScreenDialog("Select Temperature");
screen::ScreenDialog screen_unload_pull = screen::ScreenDialog("Press to abort");
screen::ScreenMenu screen_unload_retry = screen::ScreenMenu("Push to continue");
//Load Filament screens
screen::ScreenSelector screen_load_select = screen::ScreenSelector("Dismount filament");
screen::ScreenDialog screen_load_heating = screen::ScreenDialog("Select Temperature");
screen::ScreenDialog screen_load_pull = screen::ScreenDialog("Press to abort");
screen::ScreenMenu screen_load_retry = screen::ScreenMenu("Push to continue");
//Level Plate screens
screen::ScreenMenu screen_level_confirm = screen::ScreenMenu("Level Plate");
screen::ScreenDialog screen_level1 = screen::ScreenDialog("Screen1");
screen::ScreenDialog screen_level2 = screen::ScreenDialog("Screen2");
screen::ScreenDialog screen_level3 = screen::ScreenDialog("Screen3");
screen::ScreenDialog screen_level4 = screen::ScreenDialog("Screen4");
screen::ScreenMenu screen_level_retry = screen::ScreenMenu("Push to Cotinue");
//AutoHome screen
screen::ScreenMenu screen_autohome = screen::ScreenMenu("Auto-home");
//Stepper screen
screen::ScreenMenu screen_stepper = screen::ScreenMenu("Steper on");
//Move Axis screens
screen::ScreenMenu screen_move = screen::ScreenMenu("Move axis");
screen::ScreenMenu screen_move_back2main = screen::ScreenMenu("Back");
screen::ScreenMenu screen_move_x = screen::ScreenMenu("Move X");
screen::ScreenMenu screen_move_y = screen::ScreenMenu("Move Y");
screen::ScreenMenu screen_move_z = screen::ScreenMenu("Move Z");
screen::ScreenMenu screen_move_e = screen::ScreenMenu("Move Extruder");
screen::ScreenMenu screen_move_back2move = screen::ScreenMenu("Back");
screen::ScreenSelector screen_move_10 = screen::ScreenSelector("Move 10mm");
screen::ScreenSelector screen_move_1 = screen::ScreenSelector("Move 1mm");
screen::ScreenSelector screen_move_01 = screen::ScreenSelector("Move 01mm");
//Temperature screen
screen::ScreenSelector screen_temperature = screen::ScreenSelector("Temp 0/200ºC");
//Light screen
screen::ScreenMenu screen_light = screen::ScreenMenu("Led light on");
//Info screen
screen::ScreenDialog screen_info = screen::ScreenDialog("FW info");


screen::Screen * active_view;
screen::Screen * parent_view;

// Only for new LCD
typedef struct {
    uint8_t type;
    char text[18];
    func_p action;
    void * data;
} cache_entry;

// Only for new LCD
typedef struct {
    char filename[13];
    char longFilename[LONG_FILENAME_LENGTH];
} cache_entry_data;

/*******************************************************************************
**   Variables
*******************************************************************************/

// Extern variables
extern bool stop_buffer;
extern int stop_buffer_code;
extern uint8_t buffer_recursivity;

static float manual_feedrate[] = MANUAL_FEEDRATE;

// Configuration settings
// TODO : Review & delete if possible
int plaPreheatHotendTemp;
int plaPreheatHPBTemp;
int plaPreheatFanSpeed;

int absPreheatHotendTemp;
int absPreheatHPBTemp;
int absPreheatFanSpeed;

uint8_t prev_encoder_position;

float move_menu_scale;

//Filament loading/unloading
const uint8_t NORMAL = 0;
const uint8_t LOADING = 1;
const uint8_t UNLOADING = 2;
uint8_t filamentStatus = NORMAL;


// Display related variables
uint8_t    display_refresh_mode;
uint32_t   display_time_refresh = 0;

uint32_t   display_timeout;
bool       display_timeout_blocked;

view_t display_view_next;
view_t display_view;

uint32_t refresh_interval;

// Encoder related variables
uint8_t encoder_input;
uint8_t encoder_input_last;
bool    encoder_input_blocked;
bool    encoder_input_updated;

int16_t encoder_position;


// Button related variables
uint8_t button_input;
uint8_t button_input_last;
bool    button_input_blocked;
bool    button_input_updated;

bool    button_clicked_triggered;

// Beeper related variables
bool beeper_level = false;
uint32_t beeper_duration = 0;
uint8_t beep_count, frequency_ratio;

// ISR related variables
uint16_t lcd_timer = 0;

// Status screen drawer variables
uint8_t lcd_status_message_level;
char lcd_status_message[LCD_WIDTH+1] = WELCOME_MSG;


// View drawers variables
uint8_t display_view_menu_offset = 0;
uint8_t display_view_wizard_page = 0;


// SD cacheable menu variables
cache_entry cache[MAX_CACHE_SIZE];
cache_entry_data cache_data[MAX_CACHE_SIZE];
const cache_entry* cache_index = &cache[0];

bool cache_update = false;
bool cache_menu_first_time = false;
bool folder_is_root = false;

int8_t list_length;
uint8_t window_size, cache_size;
int8_t item_selected, new_item_selected;
int8_t window_min, window_max, cache_min, cache_max;

uint8_t window_offset, cursor_offset;


// TODO : Review & delete if possible
// Variables used when editing values.
const char* editLabel;
void* editValue;
int32_t minEditValue, maxEditValue;

#    if (SDCARDDETECT > 0)
bool lcd_oldcardstatus;
#    endif // (SDCARDDETECT > 0)
// 

// TODO : Review structure of this file to include it above.
#ifdef DOGLCD
#include "dogm_lcd_implementation.h"
#else
#include "ultralcd_implementation_hitachi_HD44780.h"
#endif
//






/*******************************************************************************
**   Function definitions
*******************************************************************************/

//
// General API definitions
// 

void lcd_init()
{
    // Low level init libraries for lcd & encoder
    lcd_implementation_init();

    pinMode(BTN_EN1,INPUT);
    pinMode(BTN_EN2,INPUT);
    WRITE(BTN_EN1,HIGH);
    WRITE(BTN_EN2,HIGH);

    pinMode(BTN_ENC,INPUT);
    WRITE(BTN_ENC,HIGH);

    // Init for SD card library
#  if (defined (SDSUPPORT) && defined(SDCARDDETECT) && (SDCARDDETECT > 0))
    pinMode(SDCARDDETECT,INPUT);
    WRITE(SDCARDDETECT, HIGH);
    lcd_oldcardstatus = IS_SD_INSERTED;
#  endif // (defined (SDSUPPORT) && defined(SDCARDDETECT) && (SDCARDDETECT > 0))

    // Init Timer 5 and set the OVF interrupt (triggered every 125 us)
    TCCR5A = 0x03;
    TCCR5B = 0x19;

    OCR5AH = 0x07;
    OCR5AL = 0xD0;

    lcd_enable_interrupt();

    display_time_refresh = millis();
    display_timeout = millis();
    refresh_interval = millis();
    lcd_enable_display_timeout();

    lcd_enable_button();
    lcd_get_button_updated();

    lcd_enable_encoder();
    lcd_get_encoder_updated();
    encoder_position = 0;

    lcd_get_button_clicked();

    //Create screens
    //SD card screens
    screen_main.add(screen_SD_list);
    screen_SD_list.add(screen_SD_confirm);
    screen_SD_confirm.add(screen_SD_back);
    screen_SD_confirm.add(screen_SD_continue);
    screen_SD_back.add(screen_SD_list);
    screen_SD_continue.add(screen_main); 
    //Unload filament screens
    screen_main.add(screen_unload_select);
    screen_unload_select.add(screen_main);
    screen_main.add(screen_load_select);
    screen_load_select.add(screen_main);
    screen_main.add(screen_level_confirm);
    screen_level_confirm.add(screen_main);
    screen_main.add(screen_autohome);
    screen_autohome.add(screen_main);
    screen_main.add(screen_stepper);
    screen_stepper.add(screen_main);
    screen_main.add(screen_move);
    screen_move.add(screen_main);
    screen_main.add(screen_temperature);
    screen_temperature.add(screen_main);
    screen_main.add(screen_light);
    screen_light.add(screen_main);
    screen_main.add(screen_info);
    screen_info.add(screen_main);

    active_view = new screen::ScreenMenu(screen_main);
    active_view->draw();
    
    SERIAL_ECHOLN("LCD initialized!");
}


// Interrupt-driven functions
static void lcd_update_button()
{
    static uint16_t button_pressed_count = 0;

    // Read the hardware
    button_input = lcd_implementation_update_buttons();

    // Process button click/keep-press events
    bool button_clicked = ((button_input & EN_C) && (~(button_input_last) & EN_C));
    bool button_pressed = ((button_input & EN_C) && (button_input_last & EN_C));

    if (button_clicked == true)
    {
        active_view = &active_view->press();
    }

    if (button_pressed == true) {
        button_pressed_count++;        
    } else { 
        button_pressed_count = 0;
    }

    // Beeper feedback
    if ((button_clicked == true) || (button_pressed_count > 50)) 
    {
        lcd_implementation_quick_feedback();
	//if (button_pressed_count == 200)
	//    lcd_emergency_stop();
    }

    // Update button trigger
    if ((button_clicked == true) && (button_input_blocked == false)) {
        button_clicked_triggered = true;
    }

    if ((button_input != button_input_last) && (button_input_blocked == false)) {
        button_input_updated = true;
    }

    button_input_last = button_input;
}

static void lcd_update_encoder()
{
    // Read the input hardware
    encoder_input = lcd_implementation_update_buttons();

    // Process rotatory encoder events if they are not disabled 
    if (encoder_input != encoder_input_last && encoder_input_blocked == false) {
        prev_encoder_position = encoder_position;
        switch (encoder_input & (EN_A | EN_B)) {
        case encrot0:
            if ( (encoder_input_last & (EN_A | EN_B)) == encrot3 )
                active_view->right();//encoder_position++;
            else if ( (encoder_input_last & (EN_A | EN_B)) == encrot1 )
                active_view->left();//encoder_position--;
            break;
        
        case encrot1:
            if ( (encoder_input_last & (EN_A | EN_B)) == encrot0 )
                active_view->right();//encoder_position++;
            else if ( (encoder_input_last & (EN_A | EN_B)) == encrot2 )
                active_view->left();//encoder_position--;
            break;
        
        case encrot2:
            if ( (encoder_input_last & (EN_A | EN_B)) == encrot1 )
                active_view->right();//encoder_position++;
            else if ( (encoder_input_last & (EN_A | EN_B)) == encrot3 )
                active_view->left();//encoder_position--;
            break;
        
        case encrot3:
            if ( (encoder_input_last & (EN_A | EN_B)) == encrot2 )
                active_view->right();//encoder_position++;
            else if ( (encoder_input_last & (EN_A | EN_B)) == encrot0 )
                active_view->left();//encoder_position--;
            break;
        }

        encoder_input_updated = true;
    }

    // Update the phases
    encoder_input_last = encoder_input;
}

void lcd_update(bool force)
{
#  if (SDCARDDETECT > 0)
    if ((lcd_oldcardstatus != IS_SD_INSERTED)) {
        display_refresh_mode = CLEAR_AND_UPDATE_SCREEN;
        lcd_oldcardstatus = IS_SD_INSERTED;

#ifndef DOGLCD
        lcd_implementation_init(); // to maybe revive the LCD if static electricity killed it.
#endif //DOGLCD

        if(lcd_oldcardstatus) {
            card.initsd();
            LCD_MESSAGEPGM(MSG_SD_INSERTED);
        } else {
            card.release();
            LCD_MESSAGEPGM(MSG_SD_REMOVED);
        }
    }
#  endif // (SDCARDDETECT > 0)

    //if ( display_view == view_status_screen || display_timeout_blocked || button_input_updated || encoder_input_updated ) {
    //    display_timeout = millis() + LCD_TIMEOUT_STATUS;
    //}

    //if (display_timeout < millis())
    //    lcd_set_status_screen();
    if (display_view != display_view_next) {
        display_refresh_mode = CLEAR_AND_UPDATE_SCREEN;
        lcd_clear_triggered_flags();
    }

    display_view = display_view_next;
    if ( (IS_SD_PRINTING == true) && (!force) ) {
        if (refresh_interval < millis()) {
            active_view->draw();
            refresh_interval = millis() + LCD_REFRESH_LIMIT;
        }
    } else {
        active_view->draw();
    }
}

void lcd_set_menu(view_t menu)
{
    display_view_next = menu;
    display_view_menu_offset = 0;
    
    encoder_position = 0;

    lcd_clear_triggered_flags();
}
void lcd_set_picture(view_t picture)
{
    display_view_next = picture;

    lcd_clear_triggered_flags();
}
void lcd_set_wizard(view_t wizard)
{
    display_view_next = wizard;
    lcd_wizard_set_page(0);

    lcd_clear_triggered_flags();
}

// Get and clear trigger functions
bool lcd_get_button_updated() {
    bool status = button_input_updated;
    button_input_updated = false;
    return status;
}

bool lcd_get_encoder_updated()
{
    bool status = encoder_input_updated;
    encoder_input_updated = false;
    return status;
}
bool lcd_get_button_clicked(){
    bool status = button_clicked_triggered;
    button_clicked_triggered = false;
    return status;
}
void lcd_clear_triggered_flags() {
    button_input_updated = false;
    encoder_input_updated = false;
    button_clicked_triggered = false;
}


void lcd_disable_buzzer()
{
    beep_count = 0;
    beeper_duration = 0;
    beeper_level = false;
    WRITE(BEEPER, beeper_level);
}

// Enable/disable function
void lcd_enable_button() {
    button_input = lcd_implementation_update_buttons();
    button_input_last = button_input;
    button_input_blocked = false;
}
void lcd_disable_button() {
    button_input_blocked = true;
}

void lcd_enable_encoder()
{
    encoder_input = lcd_implementation_update_buttons();
    encoder_input_last = encoder_input;
    encoder_input_blocked = false;
}
void lcd_disable_encoder()
{
    encoder_input_blocked = true;
}

void lcd_enable_display_timeout()
{
    display_timeout_blocked = false;
}
void lcd_disable_display_timeout()
{
    display_timeout_blocked = true;
}

void lcd_enable_interrupt()
{
    TIMSK5 |= 0x01;
    lcd_enable_button();
    lcd_enable_encoder();
}

void lcd_disable_interrupt()
{
    lcd_disable_button();
    lcd_disable_encoder();
    lcd_disable_buzzer();
    TIMSK5 &= ~(0x01);
}


// Control flow functions
void lcd_wizard_set_page(uint8_t page)
{
    display_view_wizard_page = page;
    display_refresh_mode = CLEAR_AND_UPDATE_SCREEN;
}


// Temporal
void lcd_beep()
{
    lcd_implementation_quick_feedback();
}

void lcd_beep_ms(uint16_t ms)
{
    frequency_ratio = 0;
    beeper_duration = 8 * ms;
    while (beeper_duration) {
        lcd_update();
    }
}

void lcd_beep_hz_ms(uint16_t frequency, uint16_t ms)
{
    frequency_ratio = (4000 / frequency) - 1;
    beeper_duration = 8 * ms;
    while (beeper_duration) {
        lcd_update();
    }
}

void lcd_set_refresh(uint8_t mode)
{
    display_refresh_mode = mode;
}

uint8_t lcd_get_encoder_position()
{
    return encoder_position;
}

#ifdef DOGLCD
void lcd_setcontrast(uint8_t value) {
    pinMode(39, OUTPUT);   //Contraste = 4.5V
    digitalWrite(39, HIGH);
}
#endif // DOGLCD

// Alert/status messages
void lcd_setstatus(const char* message)
{
    if (lcd_status_message_level > 0)
        return;
    strncpy(lcd_status_message, message, LCD_WIDTH);
}

void lcd_setstatuspgm(const char* message)
{
    if (lcd_status_message_level > 0)
        return;
    strncpy_P(lcd_status_message, message, LCD_WIDTH);
}

void lcd_setalertstatuspgm(const char* message)
{
    lcd_setstatuspgm(message);
    lcd_status_message_level = 1;

    //display_view_next = view_status_screen;

}

void lcd_reset_alert_level()
{
    lcd_status_message_level = 0;
}

static void lcd_set_encoder_position(int8_t position)
{
    encoder_position = position;
}


/********************************/
/** Float conversion utilities **/
/********************************/
//  convert float to string with +123.4 format
char conv[8];

char *ftostr3(const float &x)
{
    return itostr3((int)x);
}

char *itostr2(const uint8_t &x)
{
    //sprintf(conv,"%5.1f",x);
    int xx=x;
    conv[0]=(xx/10)%10+'0';
    conv[1]=(xx)%10+'0';
    conv[2]=0;
    return conv;
}

//  convert float to string with +123.4 format
char *ftostr31(const float &x)
{
    int xx=x*10;
    conv[0]=(xx>=0)?'+':'-';
    xx=abs(xx);
    conv[1]=(xx/1000)%10+'0';
    conv[2]=(xx/100)%10+'0';
    conv[3]=(xx/10)%10+'0';
    conv[4]='.';
    conv[5]=(xx)%10+'0';
    conv[6]=0;
    return conv;
}

//  convert float to string with 123.4 format
char *ftostr31ns(const float &x)
{
    int xx=x*10;
    //conv[0]=(xx>=0)?'+':'-';
    xx=abs(xx);
    conv[0]=(xx/1000)%10+'0';
    conv[1]=(xx/100)%10+'0';
    conv[2]=(xx/10)%10+'0';
    conv[3]='.';
    conv[4]=(xx)%10+'0';
    conv[5]=0;
    return conv;
}

char *ftostr32(const float &x)
{
    long xx=x*100;
    if (xx >= 0)
        conv[0]=(xx/10000)%10+'0';
    else
        conv[0]='-';
    xx=abs(xx);
    conv[1]=(xx/1000)%10+'0';
    conv[2]=(xx/100)%10+'0';
    conv[3]='.';
    conv[4]=(xx/10)%10+'0';
    conv[5]=(xx)%10+'0';
    conv[6]=0;
    return conv;
}

char *itostr31(const int &xx)
{
    conv[0]=(xx>=0)?'+':'-';
    conv[1]=(xx/1000)%10+'0';
    conv[2]=(xx/100)%10+'0';
    conv[3]=(xx/10)%10+'0';
    conv[4]='.';
    conv[5]=(xx)%10+'0';
    conv[6]=0;
    return conv;
}

char *itostr3(const int &x)
{
  int xx = x;
  if (xx < 0) {
     conv[0]='-';
     xx = -xx;
  } else if (xx >= 100)
    conv[0]=(xx/100)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 10)
    conv[1]=(xx/10)%10+'0';
  else
    conv[1]=' ';
  conv[2]=(xx)%10+'0';
  conv[3]=0;
  return conv;
}

char *itostr3left(const int &xx)
{
    if (xx >= 100) {
        conv[0]=(xx/100)%10+'0';
        conv[1]=(xx/10)%10+'0';
        conv[2]=(xx)%10+'0';
        conv[3]=0;
    } else if (xx >= 10) {
        conv[0]=(xx/10)%10+'0';
        conv[1]=(xx)%10+'0';
        conv[2]=0;
    } else {
        conv[0]=(xx)%10+'0';
        conv[1]=0;
    }
    return conv;
}

char *itostr4(const int &xx)
{
    if (xx >= 1000)
        conv[0]=(xx/1000)%10+'0';
    else
        conv[0]=' ';
    if (xx >= 100)
        conv[1]=(xx/100)%10+'0';
    else
        conv[1]=' ';
    if (xx >= 10)
        conv[2]=(xx/10)%10+'0';
    else
        conv[2]=' ';
    conv[3]=(xx)%10+'0';
    conv[4]=0;
    return conv;
}

//  convert float to string with 12345 format
char *ftostr5(const float &x)
{
    long xx=abs(x);
    if (xx >= 10000)
        conv[0]=(xx/10000)%10+'0';
    else
        conv[0]=' ';
    if (xx >= 1000)
        conv[1]=(xx/1000)%10+'0';
    else
        conv[1]=' ';
    if (xx >= 100)
        conv[2]=(xx/100)%10+'0';
    else
        conv[2]=' ';
    if (xx >= 10)
        conv[3]=(xx/10)%10+'0';
    else
        conv[3]=' ';
    conv[4]=(xx)%10+'0';
    conv[5]=0;
    return conv;
}

//  convert float to string with +1234.5 format
char *ftostr51(const float &x)
{
    long xx=x*10;
    conv[0]=(xx>=0)?'+':'-';
    xx=abs(xx);
    conv[1]=(xx/10000)%10+'0';
    conv[2]=(xx/1000)%10+'0';
    conv[3]=(xx/100)%10+'0';
    conv[4]=(xx/10)%10+'0';
    conv[5]='.';
    conv[6]=(xx)%10+'0';
    conv[7]=0;
    return conv;
}

//  convert float to string with +123.45 format
char *ftostr52(const float &x)
{
    long xx=x*100;
    conv[0]=(xx>=0)?'+':'-';
    xx=abs(xx);
    conv[1]=(xx/10000)%10+'0';
    conv[2]=(xx/1000)%10+'0';
    conv[3]=(xx/100)%10+'0';
    conv[4]='.';
    conv[5]=(xx/10)%10+'0';
    conv[6]=(xx)%10+'0';
    conv[7]=0;
    return conv;
}

ISR(TIMER5_OVF_vect)
{
    lcd_timer++;

#if ( defined(BEEPER) && (BEEPER > 0) )
    if (beeper_duration) {
        if (beep_count == 0) {
            beeper_level = !beeper_level;
            beep_count = frequency_ratio;
        } else {
            beep_count--;
        }
        beeper_duration--;
    } else {
        beeper_level = false;
        beep_count = 0;
    }
    WRITE(BEEPER, beeper_level);    // Tone: 4 KHz (every 125 us)
#endif // ( defined(BEEPER) && (BEEPER > 0) )

    if (lcd_timer % 4 == 0)     // Every 500 us
        lcd_update_encoder();

    if (lcd_timer % 80 == 0) {  // Every 10 ms
        lcd_update_button();
        lcd_timer = 0;
    }
}