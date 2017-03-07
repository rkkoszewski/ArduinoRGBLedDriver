// Arduino RGB Light Controller
// Author: Robert Koszewski
// License: MIT License
 
#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3

//#define COLOR_FADESTEPS_DEFAULT 350         // make this higher to slow down
#define COLOR_FADESTEPS_DEFAULT 500         // make this higher to slow down
#define BRIGHTNESS_FADESTEPS_DEFAULT 10000  // make this higher to slow down

// Gamma Table
bool gamma_enabled = true;
const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };  

// Global Variables
int r, g, b;
int r_target, g_target, b_target;
int r_target_raw, g_target_raw, b_target_raw;
int r_initial, g_initial, b_initial;
int r_maxwhite, g_maxwhite, b_maxwhite;
int brightness, brightness_target, brightness_initial, brightness_desired;
int color_fade_step;
int color_fade_step_total;
int brightness_fade_step;
int brightness_fade_step_total;

unsigned int mode;
 
void setup() {
  // Setup Pins
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  
  // Start Serial Connection
  Serial.begin(57600); // 9600 Better?

  // Initial: Colors
  r = r_initial = g = g_initial = b = b_initial = 0;
  r_target = r_target_raw = 100;
  g_target = g_target_raw = 100;
  b_target = b_target_raw = 100;

  // Initial: Fadestep -> Color
  color_fade_step = 0;
  color_fade_step_total = COLOR_FADESTEPS_DEFAULT;

  // Initial: Brightness
  brightness_desired = 255; // Initial Max Brightness
  brightness = brightness_initial = 0;
  brightness_target = 0;

  // Initial: Fadestep -> Color
  brightness_fade_step = 0;
  brightness_fade_step_total = BRIGHTNESS_FADESTEPS_DEFAULT;

  // Initial Mode
  mode = 1;

  // Initial: Color Calibration
  r_maxwhite = g_maxwhite = b_maxwhite = 255;
}

/*
void serialEvent()
{
  Serial.println("GOTIT:"+ Serial.readString());
}
*/

String inString = "";

// Set Brightness
void setBrightness(byte new_brightness){
  brightness_initial = brightness; // Save Initial Brightness State
  brightness_target = new_brightness; // Set Brightness Target
  brightness_fade_step = 0; // Reset Brightness Transition
}

// Set Colors
void setColors(unsigned long color){
  // Set Target Colors
  r_target_raw = (color >> 16) & 0xff;  // next byte, counting from left: bits 16-23
  g_target_raw = (color >> 8) & 0xff;  // next byte, bits 8-15
  b_target_raw = color & 0xff;  // low-order byte: bits 0-7

  // Update Colors
  updateColors();
}

// Update Colors
void updateColors(){
  // Save Current Color State
  r_initial = r;
  g_initial = g;
  b_initial = b;
  
  // Apply Gamma
  if(gamma_enabled){
    r_target_raw = pgm_read_byte(&gamma8[r_target_raw]);
    g_target_raw = pgm_read_byte(&gamma8[g_target_raw]);
    b_target_raw = pgm_read_byte(&gamma8[b_target_raw]);
  }

  // Process Max Brightness
  r_target = filterMaxWhite(r_target_raw, r_maxwhite);
  g_target = filterMaxWhite(g_target_raw, g_maxwhite);
  b_target = filterMaxWhite(b_target_raw, b_maxwhite);
  
  // Initialize Transition
  color_fade_step = 0; // Reset Color Transition
}

// Read From Serial
void readFromSerial(){
   while (Serial.available() > 0) {
      char inChar = Serial.read();
      if (isDigit(inChar)) {
        // convert the incoming byte to a char
        // and add it to the string:
        inString += (char)inChar;
      }
      // if you get a newline, print the string,
      // then the string's value:
      if (inChar == '\n') {

        // Parse Color
        unsigned long message = inString.toInt();
        byte mode = (message >> 24) & 0xff;  // high-order (leftmost) byte: bits 24-31

        // Modes
        switch(mode){
          
          case 0: // Turn ON-OFF lights
          { 
              byte turn_lights_on = message & 0xff;  // low-order byte: bits 0-7
              if(turn_lights_on == 0){
                // Fade Out - Brightness
                setBrightness(0);
              }else{
                // Fade In - Brightness
                setBrightness(brightness_desired);
              }
          }
          break;

          case 1: // Turn ON lights (With Color Transition)
              // Set Color
              setColors(message);
              // Fade In - Brightness
              setBrightness(brightness_desired);
          break;
          
          case 5: // Set Color
              setColors(message);
          break;

          case 10: // Set Brightness
              brightness_desired = message & 0xff;
              setBrightness(brightness_desired);
          break;

          case 11: // Calibrate COLORs (Set Max White Point)
              r_maxwhite = (message >> 16) & 0xff;  // next byte, counting from left: bits 16-23
              g_maxwhite = (message >> 8) & 0xff;  // next byte, bits 8-15
              b_maxwhite = message & 0xff;  // low-order byte: bits 0-7
              // Update Colors
              updateColors();
          break;

          case 20: // ENABLE-DISABLE Fading Transitions
          {    
              byte enable_transitions = message & 0xff;  // low-order byte: bits 0-7
              if(enable_transitions == 0){
                color_fade_step_total = 1;
              }else{
                color_fade_step_total = COLOR_FADESTEPS_DEFAULT;
              }
          }
          break;

          case 21: // ENABLE-DISABLE Gamma Correction
          {    
              byte enable_gamma = message & 0xff;  // low-order byte: bits 0-7
              if(enable_gamma == 0){
                gamma_enabled = false;
              }else{
                gamma_enabled = true;
              }
              // Update Colors
              updateColors();
          }
          break;
        }

        // Serial.println("R:"+ String(r) +" | G:"+ String(g) +" | B:"+ String(b) +" | Mode:" + String(mode));
        
        // clear the string for new input:
        inString = "";
      }
    }  
}

// Process Color Transition
int processTransition(int current_color, int target_color, int color_fade_step, int total_steps){
  if(total_steps <= 1)
    return target_color;
  
  if(current_color == target_color){
    return current_color;
    
  }else if(color_fade_step <= 1){ 
    return current_color;
    
  }else if(color_fade_step >= total_steps){
    return target_color;
  }

  if(current_color < target_color){
    // Increase
    //Serial.println("Increase: " + String(target_color) + " - " +String(color_fade_step)+" / "+String(total_steps));
    return  current_color + (((float)color_fade_step) / (float)total_steps) * (target_color - current_color);
  }else{
    // Decrease
    //Serial.println("Decrease: " + String(target_color) + " - " +String(color_fade_step)+" / "+String(total_steps));
    return current_color - ((float)color_fade_step / (float)total_steps) * (current_color - target_color);
  }
}

// Filter Max White
int filterMaxWhite(int current_color, int max_white){
    return current_color * ((float)max_white / (float)255);
}

// Brightness Filter
int filterBrightness(int current_color){
    return current_color * ((float)brightness / (float)255);
}

// Main Loop
void loop() {

  // Read from Serial
  readFromSerial();

  //Serial.println("R:"+ String(r) +" | G:"+ String(g) +" | B:"+ String(b) +" | Mode:" + String(mode));

  // Process Color Transitions
  if(color_fade_step <= color_fade_step_total){
    r = processTransition(r_initial, r_target, color_fade_step, color_fade_step_total);
    g = processTransition(g_initial, g_target, color_fade_step, color_fade_step_total);
    b = processTransition(b_initial, b_target, color_fade_step, color_fade_step_total);
    color_fade_step++;
  }

  // Brigthness Transitions
  if(brightness_fade_step <= brightness_fade_step_total){
    brightness = processTransition(brightness_initial, brightness_target, brightness_fade_step, brightness_fade_step_total);
    brightness_fade_step++;
  }

  // Set Colors
  // TODO: Change this only when an update has happened
    analogWrite(REDPIN,   filterBrightness(r));
    analogWrite(GREENPIN, filterBrightness(g));
    analogWrite(BLUEPIN,  filterBrightness(b));
}
