#include <string.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <stdio.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <stdlib.h>
#include <math.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

char network[] = "MIT";
char password[] = "";

TFT_eSPI tft = TFT_eSPI();

// Possible states: refer to state diagram for each state's meaning
enum State {BUTTON_INFO, KEYBOARD_INFO, INITIAL, PASSWORD_CONFIRMATION, CREATE_PASSWORD, WELCOME, SETUP_INSTRUCTIONS, SETTINGS, REENTER_USERNAME, ENTER_PASSWORD, ENTER_EMAIL, ACTIVE_USE, TIER_1, TIER_2, 
            SEND_EMAIL, POST_ALERT, SUCCESSFUL_POST};

// Buttons to use
const int CONFIRM_BUTTON = 45; 
const int LEFT_BUTTON = 39; 
const int DOWN_BUTTON = 38;
const int RIGHT_BUTTON = 37;

// User's information
char username[16];
char new_username[16];
char email_recipient[100] = "";
char user_password[31] = "";
char attempted_user_password[31] = "";

// The letter currently selected by the user
int selected_letter[2];
int prev_selected_letter[2];
bool capitalize = false;
// Which state the user is trying to go to
uint8_t curr_selection;
// Arrays storing the positions to draw/erase the arrows from
uint16_t setup_selection[3][2] = {{0, 40}, {0, 55}, {0, 70}};
uint16_t settings_selection[3][2] = {{0, 40}, {0, 50}, {0, 60}};
// Letters on the keyboard
char letters[5][10] = {
  {'!', '@', '&', '-', '_', '(', ')', ',', '.', ';'},
  {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
  {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
  {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'S'},
  {'z', 'x', 'c', 'v', 'b', 'n', 'm', ' ', 'B', '~'}
};
int letter_arrow_positions[5][10][2];

State curr_state;
State prev_state;

// For the button class
enum button_state {S0,S1,S2,S3,S4};

// Buzzer
uint8_t AUDIO_TRANSDUCER = 14;
uint8_t AUDIO_PWM = 1;
uint32_t siren_timer;

// LED
const int redled = 2;
const int greenled = 3;
const int blueled = 4;
bool rState = 0;
bool bState = 1;

// MP3 Player
SoftwareSerial mySoftwareSerial(18, 17);
DFRobotDFPlayerMini myDFPlayer;
uint8_t audio_to_play = 16;

//Making POST requests
const int USER_INFO_RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int USER_INFO_INFO_POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t USER_INFO_IN_BUFFER_SIZE = 2000; //size of buffer to hold HTTP request
const uint16_t USER_INFO_OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
char user_info_request_buffer[USER_INFO_IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char user_info_response_buffer[USER_INFO_OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

// User authentication variables
const uint16_t VERIFY_INFO_OUT_BUFFER_SIZE = 2000;
StaticJsonDocument<2048> user_info_verify_doc;
char password_match_result[VERIFY_INFO_OUT_BUFFER_SIZE];
char user_existence[VERIFY_INFO_OUT_BUFFER_SIZE];

// Setup page information
const uint8_t PAGE_NUMBER = 16;
const char setup_pages[PAGE_NUMBER][300] = {"Welcome! Before you\ncan begin using your\ndevice, there are a\nfew simple steps you\nmust take in terms of\nsetup. Your brand new\ndevice operates in 3\nseparate tiers,\ndesigned to maximize\nyour personal safety:\nin the first tier,\n\n", 
"the device triggers a\npolice siren and\nalarm; in the second\ntier, the device\nperforms a fake phone\ncall; in the third\ntier, the device\ninforms all nearby\nonline users of\ndanger by displaying\nthe location of all\nthreats at a given\nmoment in time.\n\n", 
"Upon navigating to\nthe 'Settings Page'\nfrom your home\ndirectory, you are\nable to update your\nEMERGENCY CONTACT,\nchange your username,\nor change your\npassword. The\nEMERGENCY CONTACT\nis needed for the\nautomatic email send.\n\n", 
"You are also able to\nchange your username\nand password at any\ntime on this page.\nAlternatively, you\nmay create a new\naccount and update\ninformation on a web\nbrowser at the\nfollowing link:\nhttps://safetyalertsy\nstem.github.io/index\n.html.\n\n", 
"To activate the\nEMERGENCY SIREN,\nwhich consists of an\nalarm and flashing\nred/blue lights,\nsimply hit the LEFT\nBUTTON of your\ndevice. This mock\npolice siren is\nintelligently\ndesigned to\nintimidate\nattackers.\n\n", 
"While the siren\nis blaring, hitting\nthe DOWN BUTTON will\ninitiate a FAKE PHONE\nCALL. Hit the right\nbutton to post an\nalert.\nAlternatively, to\nturn the siren off,\nhit the CONFIRM\nBUTTON.\n\n", 
"Upon initiation,\nthe FAKE PHONE CALL\noperates through\npre-recordings that\nwere made to simulate\na realistic phone\nconversation,\ncarefully constructed\nto leave space for\nopen-ended responses\non your end while\ncontinuing to carry\nthe conversation\nforward.\n\n", 
"Press the DOWN BUTTON\nto move to the next\npre-recorded\nresponse.\n\nWe have 3 long\npre-recorded stories\nyou can invoke to\npass time.\n\n", 
"Moreover, if you\nstill feel unsafe\nafter the stories\nhave finished, you\ncan continue talking,\nand the device will\nplay short generic\nfiller responses to\ndrive the\nconversation.\n\n",
"If you short\npress the confirm\nbutton, your location\nis emailed to your\ninputed EMERGENCY\nCONTACT.\nAdditionally, the\ndevice will play an\naudio that makes it\nsound like someone\nis meeting you.\n\n",
"If you feel safe\nenough to end the\ncall, you may do so\nby long pressing\nthe CONFIRM BUTTON.\nHitting the LEFT\nBUTTON will bring\nyou back to the\nfirst tier, and\nhitting the RIGHT\nBUTTON will bring\nyou to the final\ntier.\n\n",
"The final defense\ntier is the AMBER\nALERT SYSTEM. This\nsystem visually\ndisplays all people\nwho are in danger\nand within a radius\nof 1/4 mile near you\nat a given moment\nin time, updating\nthe display every 30\nseconds.\n\n",
"You can arrive at\nthis state manually\nby hitting the RIGHT\nBUTTON from any of\nthe other states. The\ndevice can also\narrive at this\nstate automatically.\n\n",
"Upon detecting an\nunusually prolonged\nincrease in heart\nrate through your\nvelcro wristband,\nthe device\nimmediately initiates\nthe AMBER ALERT\nSYSTEM.\n\n",
"In case the GPS\ncannot find a signal,\nyou can manually\nterminate the alert\nposting process by\npressing the CONFIRM\nBUTTON. If you wish\nto remove yourself\nfrom the database,\nlog off from the\ndevice by long\npressing the CONFIRM\nBUTTON in the ACTIVE\nUSE state.\n\n",
"You are now ready\nto use your device!\nWe commend you for\nyour commitment to\ncultivating safer,\nmore confident\ncommunities.\n\n"};
uint8_t curr_page;

// Amber alert system/GPS
uint32_t amber_timer;
uint32_t successful_post_timer;
const int GPS_BUFFER_LENGTH = 200;  //size of char array we'll use for
char buffer[GPS_BUFFER_LENGTH] = {0}; //dump chars into the 
const int GPS_RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GPS_POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t GPS_IN_BUFFER_SIZE = 2000; //size of buffer to hold HTTP request
char gps_request_buffer[GPS_IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char gps_response_buffer[VERIFY_INFO_OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
bool posted_to_amber = false;
bool success = false;
// For plotting the radar on the ESP32
char lat_lon[200] = {0}; //{0};
char x_scaled_pt[20] = "";
char y_scaled_pt[20] = "";
float x_scaled_coord = 0.0;
float y_scaled_coord = 0.0;
int x_ind = 0;
int y_ind = 0;
char to_plot = 'x';

// Heart rate sensor variables
MAX30105 heartrate_sensor;
enum heart_state {HEART_IDLE, HEART_PEAK};
heart_state curr_heart_state;
float curr_IR, curr_heartrate;
long delta, lastBeat = 0; // Time at which the last beat occurred
int heart_flag;
int HIGH_BPM_THRESHOLD = 90;
const int HIGH_BPM_PERIOD = 3000;
unsigned long high_bpm_timer;
int heart_counter = 0;
bool threshold_set = false;
float heartrate_data[50];
int heartrate_data_index = 0;
float avg_heartrate;
// To not repeatedly spam the AMBER alert tier if the user's heart rate exceeds the threshold
int heart_post_timer;

class Button{
  public:
    uint32_t S2_start_time;
    uint32_t button_change_time;    
    uint32_t debounce_duration;
    uint32_t long_press_duration;
    uint8_t pin;
    uint8_t press_type;
    uint8_t button_pressed;
    button_state state; 
    Button(int p) {
      press_type = 0;  
      state = S0;
      pin = p;
      S2_start_time = millis(); //init
      button_change_time = millis(); //init
      debounce_duration = 10;
      long_press_duration = 1000;
      button_pressed = 0;
    }
    void read() {
      uint8_t button_val = digitalRead(pin);  
      button_pressed = !button_val; // invert button
    }
    int update() {
      /* Returns the button press type:
       * 0 = no current finished button press
       * 1 = short button press
       * 2 = long button press
       */
      read();
      press_type = 0;
      switch (state) {
        case S0:
          if (button_pressed) {
            state = S1; 
            button_change_time = millis();
          }
          break;
        case S1: 
          if (!button_pressed) {
            state = S0;
            button_change_time = millis();
          }
          else {
            if (millis() - button_change_time >= debounce_duration) {
              state = S2;
              S2_start_time = millis();
            }
          }
          break;
        case S2:
          if (button_pressed) {
            if (millis() - button_change_time >= debounce_duration) {
              state = S3;
            }
          }
          else {
            state = S4;
            button_change_time = millis();
          }
          break;
        case S3:
          if (!button_pressed) {
            state = S4;
            button_change_time = millis();
          }
          break;
        case S4:
          if (!button_pressed) {
            if (millis() - button_change_time >= debounce_duration) {
              if (millis() - S2_start_time >= long_press_duration) {
                press_type = 2;
              }
              else {
                press_type = 1;
              }
              state = S0;
            }
          }
          else {
            if (millis() - button_change_time < long_press_duration) {
              state = S2;
              button_change_time = millis();
            }
            else {
              state = S3;
              button_change_time = millis();
            }
          }
          break;
      }
      return press_type;
    }
};

// Buttons used by our ESP: CONFIRM - PIN 45; LEFT - PIN 39; DOWN - PIN 38; RIGHT - PIN 37
Button c_button(CONFIRM_BUTTON);
Button l_button(LEFT_BUTTON);
Button d_button(DOWN_BUTTON);
Button r_button(RIGHT_BUTTON);

// The result returned from calling update() for each of the corresponding buttons
uint8_t c_press;
uint8_t l_press;
uint8_t d_press;
uint8_t r_press;

void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(115200); 
  Serial1.begin(9600,SERIAL_8N1,12,11);

  if (heartrate_sensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 Connected!");
  } else {
    Serial.println("MAX30102 Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

  heartrate_sensor.setup(); //Configure sensor with default settings
  heartrate_sensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  heartrate_sensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  curr_heart_state = HEART_IDLE;
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  delay(1000);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    
    Serial.println(myDFPlayer.readType(),HEX);
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms

  myDFPlayer.volume(20);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  int mp3_delayms=100;

  // WiFi setup
  WiFi.begin(network, password);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                            WiFi.localIP()[1],WiFi.localIP()[0], 
                                          WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  
  // TFT setup
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background

  pinMode(CONFIRM_BUTTON, INPUT_PULLUP); 
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);

  pinMode(AUDIO_TRANSDUCER, OUTPUT);
  pinMode(redled, OUTPUT);
  pinMode(greenled, OUTPUT);
  pinMode(blueled, OUTPUT);
  ledcSetup(AUDIO_PWM, 200, 12);
  ledcWrite(AUDIO_PWM, 0); 
  ledcAttachPin(AUDIO_TRANSDUCER, AUDIO_PWM);

  curr_state = BUTTON_INFO;
  prev_state = WELCOME;

  digitalWrite(greenled, HIGH);
  digitalWrite(blueled, HIGH);
  digitalWrite(redled, HIGH);
}

void loop() {
  c_press = c_button.update();
  l_press = l_button.update();
  d_press = d_button.update();
  r_press = r_button.update();

  // No matter what state the user is in, continue to calibrate/measure the user's heart rate data
  curr_IR = heartrate_sensor.getIR();
  if (checkForBeat(curr_IR)) {
    delta = millis() - lastBeat;
    lastBeat = millis();
    curr_heartrate = 60 / (delta / 1000.0);
    if (curr_heartrate < 255 && curr_heartrate > 20) {
      avg_heartrate = averaging_filter(curr_heartrate, heartrate_data, 50, &heartrate_data_index);
    }
  }
  heart_counter++;
  if (heart_counter % 1000 == 0) {
    Serial.println(avg_heartrate);
  }
  // There have been 10000 measurements so far, which is enough for the heart rate sensor to be properly calibrated
  // We determine a proper threshold (the average heartrate + 20), which is checked against in the ACTIVE USE state to determine if we want to post to the AMBER alert database
  if (heart_counter > 10000 && !threshold_set) {
    HIGH_BPM_THRESHOLD = avg_heartrate + 20;
    threshold_set = true;
    Serial.printf("The threshold is %d.", HIGH_BPM_THRESHOLD);
  }

  switch (curr_state) {
     case BUTTON_INFO:
      // Give the user basic directions on how to use the buttons
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("INSTRUCTIONS:\nThe three buttons on\ntop are the left,\ndown, and right\nbuttons. The\nbutton on the");
        tft.println("bottom is the confirm\nbutton. Each button\ncan be pressed\nbriefly or held,\nmaking it a long\npress.");
        tft.println("Press the buttons\nto see what each\ndoes. LONG PRESS\nthe confirm button\nto move on.");
        prev_state = curr_state;
      }
      if (l_press == 1) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Left button:\nShort press");
      }
      if (l_press == 2) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Left button:\nLong press");
      }
      if (r_press == 1) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Right button:\nShort press");
      }
      if (r_press == 2) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Right button:\nLong press");
      }
      if (d_press == 1) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Down button:\nShort press");
      }
      if (d_press == 2) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Down button:\nLong press");
      }
      if (c_press == 1) {
        tft.setCursor(0, 138, 1);
        tft.println("                                                                ");
        tft.setCursor(0, 138, 1);
        tft.println("Confirm button:\nShort press");
      }
      if (c_press == 2) {
        curr_state = KEYBOARD_INFO;
      }
      break;
    case KEYBOARD_INFO:
      // Give the user basic information on how to use the keyboard
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("To use the keyboard,\nmove the selection\narrow around to reach\nyour desired letter.\nShort press the\nconfirm button to\nselect that letter.\nLong press the\nconfirm button to\nindicate that");
        tft.println("you have finished\nentering what is\nbeing asked for.\n");
        tft.println("LONG PRESS the\nconfirm button\nto move on.");
        prev_state = curr_state;
      }
      if (c_press == 2) {
        curr_state = INITIAL;
      }
      break;
    case INITIAL:
      // Prompt the user to enter their username
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Welcome! Please enter\nyour username to get\nstarted.");
        display_keyboard(30);
        selected_letter[0] = 0;
        selected_letter[1] = 0;
        prev_selected_letter[0] = -1;
        prev_selected_letter[1] = -1;
        prev_state = curr_state;
        memset(username, 0, 16);
      }
      enter_letters(c_press, l_press, d_press, r_press, username, 15);
      if (c_press == 2) {
        curr_selection = 0;
        get_user_existence();
        bool user_in_database;
        // Checks the username against the database: if the username is inside the database, then the user is a returning user trying to log in. Otherwise, they are a new user trying to create an account.
        if (strstr(user_existence, "true") != NULL) {
          user_in_database = true;
        }
        else {
          user_in_database = false;
        }
        if (!user_in_database) {
          curr_state = CREATE_PASSWORD;
        }
        else {
          curr_state = PASSWORD_CONFIRMATION;
        }
      }
      break;
    case CREATE_PASSWORD:
      // If the user is a new user, they must create a password for their account
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Please create a new\npassword.");
        display_keyboard(30);
        selected_letter[0] = 0;
        selected_letter[1] = 0;
        prev_selected_letter[0] = -1;
        prev_selected_letter[1] = -1;
        prev_state = curr_state;
        memset(user_password, 0, 31);
      }
      enter_letters(c_press, l_press, d_press, r_press, user_password, 30);
      if (c_press == 2) {
        post_new_user_info();
        curr_state = WELCOME;
      }
      break;
    case PASSWORD_CONFIRMATION:
      // If the user is a returning user, they must log in to their account with the correct password
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Enter your password\nto log in.");
        display_keyboard(30);
        selected_letter[0] = 0;
        selected_letter[1] = 0;
        prev_selected_letter[0] = -1;
        prev_selected_letter[1] = -1;
        prev_state = curr_state;
        memset(attempted_user_password, 0, 31);
      }
      enter_letters(c_press, l_press, d_press, r_press, attempted_user_password, 30);
      if (c_press == 2) {
        test_password_matches();
        bool password_matches;
        if (strstr(password_match_result, "true") != NULL) {
          password_matches = true;
        }
        else {
          password_matches = false;
        }
        // If the user entered the wrong password, we have them re-enter their password until they get it correct
        if (!password_matches) {
          prev_state = INITIAL;
          curr_state = PASSWORD_CONFIRMATION;
        }
        else {
          get_up_to_date_user_info();
          strcpy(user_password, attempted_user_password);
          strcpy(email_recipient, user_info_verify_doc["recipient"]); 
          curr_state = WELCOME;
        }
      }
      break;
    case WELCOME:
      // Basic welcome state, allowing the user to read setup instructions, change their preferences, or start using the device
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.printf("Welcome, %s!\n", username);
        tft.println("Choose one of the\nfollowing selections\nto get started.");
        tft.setCursor(12, 40, 1);
        tft.println("Setup Instructions");
        tft.setCursor(12, 55, 1);
        tft.println("Settings");
        tft.setCursor(12, 70, 1);
        tft.println("Start");
        display_selection_arrow(setup_selection[curr_selection][0], setup_selection[curr_selection][1]);
        prev_state = curr_state;
      }
      if (d_press == 1 || d_press == 2) {
        // The user has changed the option that they want to select
        erase_selection_arrow(setup_selection[curr_selection][0], setup_selection[curr_selection][1]);
        curr_selection++;
        curr_selection %= 3;
        display_selection_arrow(setup_selection[curr_selection][0], setup_selection[curr_selection][1]);
      }
      if (c_press == 1 || c_press == 2) {
        switch (curr_selection) {
          case 0:
            curr_state = SETUP_INSTRUCTIONS;
            curr_page = 0;
            break;
          case 1:
            curr_state = SETTINGS;
            curr_selection = 0;
            break;
          case 2:
            if (strlen(email_recipient) != 0 && strlen(user_password) != 0) {
              curr_state = ACTIVE_USE;
            }
            // If the user has not given their email recipient, they need to set up the device properly first
            else {
              tft.setCursor(0, 90, 1);
              tft.println("You have not yet set\nup the device. Please\nview setup instruc-\ntions first.");
            }
            break;
        }
      }
      break;
    case SETUP_INSTRUCTIONS:
      // Displays setup instructions page by page
      if (curr_state != prev_state) {
        display_page();
        prev_state = curr_state;
      } 
      if (d_press == 1 || d_press == 2) {
        // Go to the next page of instructions
        curr_page++;
        if (curr_page >= PAGE_NUMBER) {
          curr_page--;
        }
        display_page();
      }
      if (c_press == 1 || c_press == 2) {
        curr_state = WELCOME;
      }
      break;
    case SETTINGS:
      // Uses the same selection mechanism as the welcome screen to allow the user to change their settings
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Settings");
        tft.setCursor(12, 40, 1);
        tft.println("Re-enter Username");
        tft.setCursor(12, 50, 1);
        tft.println("Email Recipient");
        tft.setCursor(12, 60, 1);
        tft.println("Enter New Password");
        display_selection_arrow(settings_selection[curr_selection][0], settings_selection[curr_selection][1]);
        prev_state = curr_state;
      }
      if (d_press == 1 || d_press == 2) {
        erase_selection_arrow(settings_selection[curr_selection][0], settings_selection[curr_selection][1]);
        curr_selection++;
        curr_selection %= 3;
        display_selection_arrow(settings_selection[curr_selection][0], settings_selection[curr_selection][1]);
      }
      if (c_press == 1) {
        switch(curr_selection) {
          case 0:
            curr_state = REENTER_USERNAME;
            break;
          case 1:
            curr_state = ENTER_EMAIL;
            break;
          case 2:
            curr_state = ENTER_PASSWORD;
            break;
        }
      }
      else if (c_press == 2) {
        curr_selection = 0;
        curr_state = WELCOME;
      }
      break;
    case REENTER_USERNAME:
      // Allows the user to change their username to a new username, as long as the username has not been taken
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Entering username:");
        display_keyboard(20);
        selected_letter[0] = 0;
        selected_letter[1] = 0;
        prev_selected_letter[0] = -1;
        prev_selected_letter[1] = -1;
        prev_state = curr_state;
        memset(new_username, 0, 16);
      }
      enter_letters(c_press, l_press, d_press, r_press, new_username, 15);
      if (c_press == 2) {
        curr_selection = 0;
        update_username();
        curr_state = SETTINGS;
      }
      break;
    case ENTER_PASSWORD:
      // Allows the user to change their password
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Enter new password:");
        display_keyboard(20);
        selected_letter[0] = 0;
        selected_letter[1] = 0;
        prev_selected_letter[0] = -1;
        prev_selected_letter[1] = -1;
        prev_state = curr_state;
        memset(user_password, 0, 16);
      }
      enter_letters(c_press, l_press, d_press, r_press, user_password, 30);
      if (c_press == 2) {
        curr_selection = 0;
        update_user_info();
        curr_state = SETTINGS;
      }
      break;
    case ENTER_EMAIL:
      // Allows the user to enter their desired email recipient
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("Entering email\nrecipient:");
        display_keyboard(20);
        selected_letter[0] = 0;
        selected_letter[1] = 0;
        prev_selected_letter[0] = -1;
        prev_selected_letter[1] = -1;
        prev_state = curr_state;
        memset(email_recipient, 0, 100);
      }
      enter_letters(c_press, l_press, d_press, r_press, email_recipient, 99);
      if (c_press == 2) {
        // Save email
        curr_selection = 0;
        update_user_info();
        curr_state = SETTINGS;
      }
      break;
    case ACTIVE_USE:
      // The basic state of the functioning device in use; displays the nearby radar of users in danger
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.drawCircle(64, 64, 50, TFT_GREEN);
        tft.setCursor(63, 63);
        tft.println("X");
        tft.setCursor(0, 128);
        tft.println("Your Location:");
        extractGNRMC();
        amber_timer = millis();
        heart_post_timer = millis();
        prev_state = curr_state;
      }
      // Update the user's location every 30 seconds
      if (millis() - amber_timer > 30000) {
        tft.fillScreen(TFT_BLACK);
        tft.drawCircle(64, 64, 50, TFT_GREEN);
        tft.setCursor(63, 63);
        // "X" denotes the user's location
        tft.println("X");
        tft.setCursor(0, 128);
        tft.println("Your Location:");
        extractGNRMC();
        amber_timer = millis();
      }
      heart_flag = heartrate_activation_fsm();
      if (curr_IR >= 50000 && heart_flag == 1 && millis() - heart_post_timer > 240000) {// Long peak detected and it has been at least four minutes since the last post, auto-post to the AMBER alert system
        heart_post_timer = millis();
        success = false;
        curr_state = POST_ALERT;
      }
      if (c_press == 2) {
        // If the user logs off, we make sure that we remove them from the AMBER alert database
        curr_selection = 0;
        if (posted_to_amber) {
          remove_from_amber();
        }
        curr_state = WELCOME;
      }
      if (l_press == 1 || l_press == 2) {
        curr_state = TIER_1;
      }
      if (d_press == 1 || d_press == 2) {
        audio_to_play = 16;
        curr_state = TIER_2;
      }
      if (r_press == 1 || r_press == 2) {
        success = false;
        curr_state = POST_ALERT;
      }
      break;
    case TIER_1:
      // The first tier: flashing red/blue police lights and siren sound using the buzzer
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("FIRST TIER");
        tft.println();
        tft.println("Siren & Police Lights");
        prev_state = curr_state;
      }
      first_tier(l_press);
      if (c_press == 2) {
        curr_state = ACTIVE_USE;
        ledcWriteTone(AUDIO_PWM, 0);
        digitalWrite(greenled, HIGH);
        digitalWrite(blueled, HIGH);
        digitalWrite(redled, HIGH);
      }
      if (d_press == 1 || d_press == 2) {
        audio_to_play = 16;
        curr_state = TIER_2;
        ledcWriteTone(AUDIO_PWM, 0);
        digitalWrite(greenled, HIGH);
        digitalWrite(blueled, HIGH);
        digitalWrite(redled, HIGH);
      }
      if (r_press == 1 || r_press == 2) {
        success = false;
        curr_state = POST_ALERT;
        ledcWriteTone(AUDIO_PWM, 0);
        digitalWrite(greenled, HIGH);
        digitalWrite(blueled, HIGH);
        digitalWrite(redled, HIGH);
      }
      break;
    case TIER_2:
      // The second tier: the interactive phone call state
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        // We have different displays for each audio, accounted for using a switch-case statement, in order to make clear to the user what the played audio has said and, if necessary, their possible responses
        switch (audio_to_play) {
          case 16:
            tft.println("SECOND TIER");
            tft.println();
            tft.println("Fake Phone Call:");
            tft.println("Hey! What's up?");
            tft.println();
            tft.println("User response:\nNothing much,\nhow about you?");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 15:
            tft.println("Fake Phone Call:");
            tft.println("What are you doing\nright now?");
            tft.println();
            tft.println("User response: Taking\na walk. I came back\nfrom a club meeting.");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 14:
            tft.println("Fake Phone Call:");
            tft.println("Are you on your way?");
            tft.println();
            tft.println("User response: Yeah, I should be there\nsoon.");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 13:
            tft.println("Fake Phone Call:");
            tft.println("Let me know when\nyou're here!");
            tft.println();
            tft.println("User response: Okay!");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 12:
            tft.println("Fake Phone Call:");
            tft.println("So what did you do\ntoday?");
            tft.println();
            tft.println("User response: Just\ndid a little bit of\nwork. What about you?");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 11:
            tft.println("Fake Phone Call:");
            tft.println("It's been good for\nme...");
            tft.println();
            tft.println("User response:\nSounds like a\nproductive day!");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 10:
            tft.println("Fake Phone Call:");
            tft.println("Story 1");
            tft.println();
            tft.println("User instructions:\nLaugh occasionally");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 9:
            tft.println("Fake Phone Call:");
            tft.println("Story 2");
            tft.println();
            tft.println("User instructions:\nLaugh occasionally");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 8:
            tft.println("Fake Phone Call:");
            tft.println("Story 3");
            tft.println();
            tft.println("User instructions:\nLaugh occasionally");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 7:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("User instructions:\nTalk as long as you\nwant");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 6:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("User instructions:\nTalk as long as you\nwant");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 5:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("User instructions:\nTalk as long as you\nwant");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 4:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("User instructions:\nTalk as long as you\nwant");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 3:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("User instructions:\nTalk as long as you\nwant");
            tft.println();
            tft.println("PRESS CONFIRM TO SEND\nLOCATION");
            tft.println();
            tft.println("HOLD CONFIRM TO END\nPHONE CALL");
            break;
          case 2:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("Where are you?");
            tft.println("User instructions:\nSay your location");
            break;
          case 1:
            tft.println("Fake Phone Call:");
            tft.println();
            tft.println("Goodbyes");
            break;
        }
        prev_state = curr_state;
        // When we enter this state the first time, we want to start playing the audio
        myDFPlayer.play(audio_to_play);
        // For the randomly played generic responses, we generate a pseudo-random number to determine which one to play
        if (audio_to_play > 2 && audio_to_play < 8) {
          int randNum = millis() % 5;
          audio_to_play = 3 + randNum;
        } 
        // The user just sent their location via email, so we wish to kill time by playing the long, pre-recorded stories
        else if (audio_to_play == 2) {
          audio_to_play = 10;
        }
        // The audios are played in the order of the script
        else {
          audio_to_play--;
        }
      }
      // The user has ended the phone call state
      if (c_press == 2) {
        myDFPlayer.pause();
        curr_state = ACTIVE_USE;
      }
      if (l_press == 1 || l_press == 2) {
        myDFPlayer.pause();
        curr_state = TIER_1;
      }
      // The user wants to play the next audio in the sequence
      if (d_press == 1 || d_press == 2) {
        myDFPlayer.pause();
        prev_state = ACTIVE_USE;
        curr_state = TIER_2;
      }
      if (r_press == 1 || r_press == 2) {
        myDFPlayer.pause();
        success = false;
        curr_state = POST_ALERT;
      }
      // The user feels that they are in danger and wishes to send their location to their specified email recipient
      if (c_press == 1) {
        prev_state = TIER_2;
        curr_state = SEND_EMAIL;
      }
      break;
    case SEND_EMAIL:
      // Send the email with their location to the specified recipient
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.println("Sending Email");
      send_email();
      prev_state = curr_state;
      curr_state = TIER_2;
      audio_to_play = 2;
      break;
    case POST_ALERT:
      // Post the user's location to the AMBER alert database
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("THIRD TIER");
        tft.println();
        tft.println("Posting alert...");
        prev_state = curr_state;
      }
      // Continuously try to post the AMBER alert
      post_amber_alert();
      if (success) {
        curr_state = SUCCESSFUL_POST;
        success = false;
        posted_to_amber = true;
      }
      // Pressing the confirm button indiciates that the user wishes to stop trying to POST and instead use other functions
      else if (c_press == 1 || c_press == 2) {
        success = false;
        curr_state = ACTIVE_USE;
      }
      break;
    case SUCCESSFUL_POST:
      // Show the user that their location was successfully posted
      if (curr_state != prev_state) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 1);
        tft.println("AMBER Alert Posted");
        successful_post_timer = millis();
        prev_state = curr_state;
      }
      if (millis() - successful_post_timer > 2000) {
        curr_state = ACTIVE_USE;
      }
      if (l_press == 1 || l_press == 2) {
        curr_state = TIER_1;
      }
      if (d_press == 1 || d_press == 2) {
        audio_to_play = 16;
        curr_state = TIER_2;
      }
      break;
  }
}

void display_keyboard(uint16_t starting_position) {
  /*
   * Given the starting row, in terms of pixels, of the keyboard, displays the full keyboard at that location. Updates the letter_arrow_positions array to keep track of the location
   * of each of the 49 letters and symbols included in the keyboard. 
   */
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 10; j++) {
      int horizontal_spacing;
      if (i < 3) {
        horizontal_spacing = 11;
      }
      else if (i == 3) {
        horizontal_spacing = 10;
      }
      else {
        horizontal_spacing = 9;
      }
      tft.setCursor(j*horizontal_spacing, starting_position + (i*20), 1);
      // Since letters is an array of characters, we want to replace the ' ' character with the word "Space" when printing the keyboard to make it clear to users
      if (letters[i][j] == ' ') {
        letter_arrow_positions[i][j][0] = j*horizontal_spacing;
        letter_arrow_positions[i][j][1] = starting_position + (i*20) + 9;
        tft.print("Space");
      }
      // Backspace key
      else if (letters[i][j] == 'B') {
        letter_arrow_positions[i][j][0] = j*horizontal_spacing + 26;
        letter_arrow_positions[i][j][1] = starting_position + (i*20) + 9;
        tft.setCursor(j*horizontal_spacing + 26, starting_position + (i*20), 1);
        tft.print("Bksp");
      }
      // Shift key
      else if (letters[i][j] == 'S') {
        letter_arrow_positions[i][j][0] = j*horizontal_spacing;
        letter_arrow_positions[i][j][1] = starting_position + (i*20) + 9;
        tft.setCursor(j*horizontal_spacing, starting_position + (i*20), 1);
        tft.print("Shift");
      }
      // The 50th nonexistent letter is represented by '~', which we do not want to show on the keyboard
      else if (letters[i][j] != '~') {
        letter_arrow_positions[i][j][0] = j*horizontal_spacing;
        letter_arrow_positions[i][j][1] = starting_position + (i*20) + 9;
        tft.print(letters[i][j]);
      }
    }
  }
}

void display_selection_arrow(uint16_t starting_x, uint16_t starting_y) {
  /*
   * Given the location to display the selection arrow, prints a ">" symbol to represent an arrow.
   */
  tft.setCursor(starting_x, starting_y, 1);
  tft.println(">");
}

void erase_selection_arrow(uint16_t starting_x, uint16_t starting_y) {
  /*
   * Given the location, erases the selection arrow by printing spaces at that location.
   */
  tft.setCursor(starting_x, starting_y, 1);
  tft.println("  ");
}

void display_letter_arrow(uint16_t x, uint16_t y) {
  /*
   * Given the location in pixels, uses the '^' symbol to show the user which letter they are selecting.
   */
  tft.setCursor(x, y, 1);
  tft.print("^");
}

void enter_letters(uint8_t c, uint8_t l, uint8_t d, uint8_t r, char* curr_string, uint32_t out_size) {
  /*
   * Given the keyboard presses by the user, adjusts the user's currently selected letter and the current string being entered by the user if necessary. 
   * Also displays the current string that has been entered by the user so far. 
   */
  // The user has changed the letter that they wish to select, so we need to update the display of the selection arrow to reflect this as well
  if (prev_selected_letter[0] != selected_letter[0] || prev_selected_letter[1] != selected_letter[1]) {
    if (prev_selected_letter[0] != -1) {
      erase_selection_arrow(letter_arrow_positions[prev_selected_letter[0]][prev_selected_letter[1]][0], letter_arrow_positions[prev_selected_letter[0]][prev_selected_letter[1]][1]);
    }
    display_letter_arrow(letter_arrow_positions[selected_letter[0]][selected_letter[1]][0], letter_arrow_positions[selected_letter[0]][selected_letter[1]][1]);
    prev_selected_letter[0] = selected_letter[0];
    prev_selected_letter[1] = selected_letter[1];
  }
  // Move the selection cursor to the left by 1
  if (l == 1) {
    selected_letter[1] -= 1;
    if (selected_letter[1] < 0) {
      selected_letter[1] += 10;
    }
  }
  // Move the selection cursor to the right by 1
  if (r == 1) {
    selected_letter[1] += 1;
    selected_letter[1] %= 10;
  }
  // Move the selection cursor down 1 row
  if (d == 1) {
    selected_letter[0] += 1;
    selected_letter[0] %= 5;
  }
  // Since the last letter of the last row is nonexistent, based on the button presses, we determine which letter is the correct one that we should wrap around to
  if (selected_letter[0] == 4 && selected_letter[1] == 9) {
    if (prev_selected_letter[0] == 3) {
      selected_letter[1] = 8;
    }
    else if (prev_selected_letter[1] == 8) {
      selected_letter[1] = 0; 
    }
    else {
      selected_letter[1] = 8;
    }
  }
  // When the confirm button is pressed, this is interpreted as the user selecting a letter
  if (c == 1) {
    // Check that the current string is not full if the user is adding an letter
    if (strlen(curr_string) < out_size || letters[selected_letter[0]][selected_letter[1]] == 'B') {
      // The user wants to capitalize the next letter
      if (letters[selected_letter[0]][selected_letter[1]] == 'S') {
        capitalize = true;
      }
      // The user wants to delete the last letter of the currently entered string
      else if (letters[selected_letter[0]][selected_letter[1]] == 'B') {
        int curr_len = strlen(curr_string);
        if (curr_len != 0) {
          curr_string[curr_len - 1] = '\0';
        }
      }
      // The user has entered a normal letter
      else {
        if (!capitalize) {
          strncat(curr_string, &letters[selected_letter[0]][selected_letter[1]], 1);
        }
        else {
          capitalize = false;
          // Check if the letter is one that is able to capitalized -- not a symbol, backspace, shift, or space key
          if (selected_letter[0] == 2 || (selected_letter[0] == 3 && selected_letter[1] != 9) || (selected_letter[0] == 4 && selected_letter[1] != 8 && selected_letter[1] != 9)) {
            char capitalized_letter = char(int(letters[selected_letter[0]][selected_letter[1]]) - 32);
            strncat(curr_string, &capitalized_letter, 1);
          }
          else {
            strncat(curr_string, &letters[selected_letter[0]][selected_letter[1]], 1);
          }
        }
      }
      // Clear the current displayed string and update it with the correct string
      tft.setCursor(0, 130, 1);
      tft.println("                                                                ");
      tft.setCursor(0, 140, 1);
      tft.println("                                                                ");
      tft.setCursor(0, 130, 1);
      tft.println(curr_string);
    }
    else {
      tft.setCursor(0, 140, 1);
      tft.println("Reached maximum\ncharacter limit.");
    }
  }
}

void first_tier(uint8_t l) {
  /*
   * The main body of code for carrying out the first tier's functionality. Called repeatedly in the main loop when the user is in the first tier.
   * Uses timers to keep track of when to change the LED color as well as to modify the frequencies played by the buzzer to imitate a real police siren.
   */
  digitalWrite(greenled, HIGH);
  if (millis() - siren_timer > 500) {
    digitalWrite(redled, rState? HIGH : LOW);
    digitalWrite(blueled, bState? HIGH : LOW);
    rState = !rState;
    bState = !bState;
    siren_timer = millis();
  }

  int loopedTimer = millis() % 4000;
  if(loopedTimer < 2000) {
    ledcWriteTone(AUDIO_PWM, (500 + (int)(loopedTimer/2)));
  } else {
    ledcWriteTone(AUDIO_PWM, (2500 - (int)(loopedTimer/2)));
  }
}

void display_page() {
  /*
   * Called in the SETUP_INSTRUCTIONS page to display each page of the setup instructions with the proper formatting and instructions for moving on to the next page of
   * the instructions. 
   */
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 1);
  tft.printf("Page %d\n", curr_page + 1);
  tft.print(setup_pages[curr_page]);
  if (curr_page != PAGE_NUMBER - 1) {
    tft.print("Press the down button\nto go the the next\npage.");
  }
  else {
    tft.print("Press the confirm\nbutton to return to\nthe main menu.");
  }
}

void send_email() {
  /*
   * Generates and performs the POST request to the user information server file in order to send out the email containing the user's current location.
   */
  char body[200];
  sprintf(body,"user=%s&location=%s&action=send_email",username,lat_lon,user_password,username);
  int body_len = strlen(body); //calculate body length (for header reporting)
  sprintf(user_info_request_buffer,"POST http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py HTTP/1.1\r\n");
  strcat(user_info_request_buffer,"Host: 608dev-2.net\r\n");
  strcat(user_info_request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(user_info_request_buffer+strlen(user_info_request_buffer),"Content-Length: %d\r\n", body_len); 
  strcat(user_info_request_buffer,"\r\n"); //new line from header to body
  strcat(user_info_request_buffer,body); //body
  strcat(user_info_request_buffer,"\r\n"); //new line
  
  do_http_request("608dev-2.net", user_info_request_buffer, user_info_response_buffer, USER_INFO_OUT_BUFFER_SIZE, USER_INFO_RESPONSE_TIMEOUT,true);
  Serial.println(user_info_response_buffer);
}

void post_new_user_info() {
  /*
   * Generates and performs the POST request for creating a new account for the user, given the user's desired username and password.
   */
  char body[1000]; 
  sprintf(body,"user=%s&password=%s&name=%s&action=set_settings&account=create",username,user_password,username);
  int body_len = strlen(body); 
  sprintf(user_info_request_buffer,"POST http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py HTTP/1.1\r\n");
  strcat(user_info_request_buffer,"Host: 608dev-2.net\r\n");
  strcat(user_info_request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(user_info_request_buffer+strlen(user_info_request_buffer),"Content-Length: %d\r\n", body_len);
  strcat(user_info_request_buffer,"\r\n");
  strcat(user_info_request_buffer,body); 
  strcat(user_info_request_buffer,"\r\n");
  
  do_http_request("608dev-2.net", user_info_request_buffer, user_info_response_buffer, USER_INFO_OUT_BUFFER_SIZE, USER_INFO_RESPONSE_TIMEOUT,true);
  Serial.println(user_info_response_buffer); //viewable in Serial Terminal
}

void update_user_info() {
  /*
   * Generates and performs the POST request for updating the settings of a user's account -- this includes any changes to the user's username or their email recipient.
   */
  char body[1000]; 
  sprintf(body,"user=%s&password=%s&name=%s&contact=%s&prefs=default&keyword=default&action=set_settings&account=update",username,user_password, username, email_recipient);
  int body_len = strlen(body);
  sprintf(user_info_request_buffer,"POST http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py HTTP/1.1\r\n");
  strcat(user_info_request_buffer,"Host: 608dev-2.net\r\n");
  strcat(user_info_request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(user_info_request_buffer+strlen(user_info_request_buffer),"Content-Length: %d\r\n", body_len); 
  strcat(user_info_request_buffer,"\r\n");
  strcat(user_info_request_buffer,body); 
  strcat(user_info_request_buffer,"\r\n"); 

  do_http_request("608dev-2.net", user_info_request_buffer, user_info_response_buffer, USER_INFO_OUT_BUFFER_SIZE, USER_INFO_RESPONSE_TIMEOUT,true);
  Serial.println(user_info_response_buffer); //viewable in Serial Terminal
}

void update_username() {
   /*
    * Generates and performs the POST request for updating a user's username. Only changes the local variable holding the user's username if the new username that they are changing to
    * has not been taken. 
    */
  char body[1000];
  sprintf(body,"user=%s&password=%s&new_user=%s&action=change_username",username,user_password, new_username);
  int body_len = strlen(body);
  sprintf(user_info_request_buffer,"POST http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py HTTP/1.1\r\n");
  strcat(user_info_request_buffer,"Host: 608dev-2.net\r\n");
  strcat(user_info_request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(user_info_request_buffer+strlen(user_info_request_buffer),"Content-Length: %d\r\n", body_len); 
  strcat(user_info_request_buffer,"\r\n");
  strcat(user_info_request_buffer,body); 
  strcat(user_info_request_buffer,"\r\n"); 
  do_http_request("608dev-2.net", user_info_request_buffer, user_info_response_buffer, USER_INFO_OUT_BUFFER_SIZE, USER_INFO_RESPONSE_TIMEOUT,true);
  if (strstr(user_info_response_buffer, "TAKEN") == NULL) {
    strcpy(username, new_username);
    Serial.println("Username changed successfully.");
  }
  else {
    Serial.println("Username taken.");
  }
  Serial.println(user_info_response_buffer);
}

void test_password_matches() {
  /*
   * Modifies the global string variable password_match_result to contain what is returned from this GET request
   */
  char request_buffer[2000];
  sprintf(request_buffer,"GET  http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py?view=false&action=match_password&user=%s&password=%s HTTP/1.1\r\n", username, attempted_user_password);
  strcat(request_buffer,"Host: 608dev-2.net\r\n");
  strcat(request_buffer,"\r\n");
  do_http_request("608dev-2.net", request_buffer, password_match_result, VERIFY_INFO_OUT_BUFFER_SIZE, 6000, true);
  Serial.println(password_match_result);
}

void get_up_to_date_user_info() {
   /*
    * Generates and performs the GET request for getting a user's stored preferences and information. This way, returning users do not need to re-enter their email recipient each time that they
    * use the device. 
    */
  char request_buffer[2000];
  char response_buffer[VERIFY_INFO_OUT_BUFFER_SIZE];
  sprintf(request_buffer,"GET  http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py?view=false&action=get_user_info&user=%s&password=%s HTTP/1.1\r\n", username, user_password);
  strcat(request_buffer,"Host: 608dev-2.net\r\n");
  strcat(request_buffer,"\r\n");
  do_http_request("608dev-2.net", request_buffer, response_buffer, VERIFY_INFO_OUT_BUFFER_SIZE, 6000, true);
  deserializeJson(user_info_verify_doc, response_buffer);
  Serial.println("Deserialized username:");
  char temp_user[100];
  strcpy(temp_user, user_info_verify_doc["user"]);
  Serial.println(temp_user);
}

void get_user_existence() {
  /*
   * Generates and performs the GET request for checking whether or not the user is one that is already inside the database. 
   */
  char request_buffer[2000];
  sprintf(request_buffer,"GET  http://608dev-2.net/sandbox/sc/team63/userinfo/server_secure.py?view=false&action=see_if_user_exists&user=%s HTTP/1.1\r\n", username);
  strcat(request_buffer,"Host: 608dev-2.net\r\n");
  strcat(request_buffer,"\r\n");
  do_http_request("608dev-2.net", request_buffer, user_existence, VERIFY_INFO_OUT_BUFFER_SIZE, 6000, true);
  Serial.println(user_existence);
}

void post_amber_alert() {
  /*
   * Tries to post the user's location to the AMBER alert database once. If successfully performed, modifies the global variable success to contain the value true, so that we do not need to continue to
   * perform this function. 
   */ 
  while (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)
    Serial1.readBytesUntil('\n', buffer, GPS_BUFFER_LENGTH); // read it and send it out Serial (USB)
    char* info = strstr(buffer,"GNRMC");
    char* v = strstr(buffer,"V");
    // These if statements only evaluate to true when the GPS has a valid reading
    if (info!=NULL){
      if (v==NULL) {
        success = true;
        Serial.println(buffer);         
        extract(buffer);

        char body[1000];
        sprintf(body,"user=%s&location=%s", username, lat_lon);
        int body_len = strlen(body);
        sprintf(user_info_request_buffer,"POST https://608dev-2.net/sandbox/sc/team63/amberalert/amber_alert_system.py HTTP/1.1\r\n");
        strcat(user_info_request_buffer,"Host: 608dev-2.net\r\n");
        strcat(user_info_request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(user_info_request_buffer+strlen(user_info_request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(user_info_request_buffer,"\r\n"); //new line from header to body
        strcat(user_info_request_buffer,body); //body
        strcat(user_info_request_buffer,"\r\n"); //new line
        do_http_request("608dev-2.net", user_info_request_buffer, user_info_response_buffer, USER_INFO_OUT_BUFFER_SIZE, USER_INFO_RESPONSE_TIMEOUT,true);
        Serial.println(user_info_response_buffer);
      }
    }
  }
}

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  /*
   * Helper function to append the character to the given string.
   */
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

void extractGNRMC(){
  /*
   * Reads the signal from the GPS and get's the user's current location. Sends up the user's location to the server, which returns a list of nearby users that need to be displayed on
   * the AMBER alert system.
   */
  bool got_location = false;
  while (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)
    Serial1.readBytesUntil('\n', buffer, GPS_BUFFER_LENGTH); // read it and send it out Serial (USB)
    char* info = strstr(buffer,"GNRMC");
    char* v = strstr(buffer,"V");
    if (info!=NULL){
      if (v==NULL) {
        Serial.println(buffer);
  
        tft.setCursor(0, 138);
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.println("                                                ");
  
        extract(buffer);
  
        tft.setCursor(0, 138);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.println(lat_lon);

        got_location = true;
  
        //GET REQUEST
        sprintf(gps_request_buffer, "GET https://608dev-2.net/sandbox/sc/team63/amberalert/amber_alert_system.py?location=%s&user=%s HTTP/1.1\r\n", lat_lon, username);
        strcat(gps_request_buffer, "Host: 608dev-2.net\r\n"); //add more to the end
        strcat(gps_request_buffer, "\r\n"); //add blank line!
        //submit to function that performs GET.  It will return output using response_buffer char array
        do_http_request("608dev-2.net", gps_request_buffer, gps_response_buffer, VERIFY_INFO_OUT_BUFFER_SIZE, GPS_RESPONSE_TIMEOUT, false);
        Serial.println(gps_response_buffer); //print to serial monitor
        
        tft.fillCircle(64, 64, 49, TFT_BLACK);
        tft.setCursor(63, 63);
        tft.println("X");
  
        //DISPLAY POINTS
        //Scale the points so that they display nearby users in the correct direction
        for(int i = 0; i < strlen(gps_response_buffer); i++){
          if(gps_response_buffer[i] == ','){
            to_plot = 'y';
          }
          else if(gps_response_buffer[i] == ';'){
            x_scaled_coord = atof(x_scaled_pt);
            y_scaled_coord = atof(y_scaled_pt);
            tft.fillCircle(x_scaled_coord, y_scaled_coord, 1, TFT_RED);
  
            memset(x_scaled_pt, strlen(x_scaled_pt), 0);
            memset(y_scaled_pt, strlen(y_scaled_pt), 0);
            x_ind = 0;
            y_ind = 0;
            to_plot = 'x';
          }
          else if(to_plot == 'x'){
            x_scaled_pt[x_ind] = gps_response_buffer[i];
            x_ind++;
          }
          else if(to_plot == 'y'){
            y_scaled_pt[y_ind] = gps_response_buffer[i];
            y_ind++;
          }
        }
      }
    }
  }
  // In case the GPS signal was not working, shows the user's location as "Loading..." until the next time that the GPS can get a signal
  if (!got_location) {
    tft.setCursor(0, 138);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.println("                                                ");
    Serial.println(buffer);
    tft.setCursor(0, 138);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Loading...");
  }
}

void extract(char* data_array){
  /*
   * Extracts the relevant location data from what is read in by the GPS.
   */
  int comma_count = 0;
  int index = 0;
  memset(lat_lon, strlen(lat_lon), 0);

  for(int i = 0; i < strlen(data_array); i++){
    if(data_array[i] ==','){
      comma_count++;
    }
    if(comma_count >= 3 && comma_count <= 6 && data_array[i] != ','){
      lat_lon[index] = data_array[i];
      index++;
    }
  }
}

void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),USER_INFO_OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}        

int heartrate_activation_fsm() {
  /*
   * The state machine for the heart rate sensor, which returns 1 if a long peak in the user's heart rate was detected. 
   */
  switch (curr_heart_state) {
    case HEART_IDLE:
      if (threshold_set && avg_heartrate > HIGH_BPM_THRESHOLD) {
        high_bpm_timer = millis();
        curr_heart_state = HEART_PEAK;
      }
      break;
    case HEART_PEAK:
      if (avg_heartrate < HIGH_BPM_THRESHOLD)
        curr_heart_state = HEART_IDLE;
      else if (millis() - high_bpm_timer > HIGH_BPM_PERIOD) // Long peak
        curr_heart_state = HEART_IDLE;
        return 1;
      break;
  }
  return 0;
}

float averaging_filter(float input, float* stored_values, int order, int* index) {
  /*
   * Computes the user's average heart rate, using the most recent set of values, the number of which is specified by order.
   */
  float sum = input;
  for (int i = 0; i < order; i++) {
    sum += stored_values[i];
  }
  sum = sum/(order + 1);
    
  int i = 0;
  if(stored_values[order - 1] == 0) {
    while(stored_values[i] != 0)
      i++;
    *index = i;
  }
  else
    *index = (*index + 1)%order;
  stored_values[*index] = input;
    
  return sum;
}

void remove_from_amber() {
  /*
   * Generates and performs the POST request to the AMBER alert file on the server that removes the user from the AMBER alert database, which is necessary for when they log off. 
   */
  char body[1000];
  sprintf(body,"user=%s",username);
  int body_len = strlen(body);
  sprintf(user_info_request_buffer,"POST http://608dev-2.net/sandbox/sc/team63/amberalert/amber_alert_remove.py HTTP/1.1\r\n");
  strcat(user_info_request_buffer,"Host: 608dev-2.net\r\n");
  strcat(user_info_request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(user_info_request_buffer+strlen(user_info_request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(user_info_request_buffer,"\r\n"); //new line from header to body
  strcat(user_info_request_buffer,body); //body
  strcat(user_info_request_buffer,"\r\n"); //new line
  do_http_request("608dev-2.net", user_info_request_buffer, user_info_response_buffer, USER_INFO_OUT_BUFFER_SIZE, USER_INFO_RESPONSE_TIMEOUT,true);
  Serial.println(user_info_response_buffer);
}
