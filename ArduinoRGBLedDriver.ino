// Arduino RGB Light Controller
// Author: Robert Koszewski
// License: MIT License
 
#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3

#define COLOR_FADESTEPS_DEFAULT 350         // make this higher to slow down
#define BRIGHTNESS_FADESTEPS_DEFAULT 10000  // make this higher to slow down

// Global Variables
int r, g, b;
int r_target, g_target, b_target;
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
  r_target = 100;
  g_target = 100;
  b_target = 100;

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
  // Save Current Color State
  r_initial = r;
  g_initial = g;
  b_initial = b;
  // Set Target Colors
  r_target = (color >> 16) & 0xff;  // next byte, counting from left: bits 16-23
  g_target = (color >> 8) & 0xff;  // next byte, bits 8-15
  b_target = color & 0xff;  // low-order byte: bits 0-7
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
        unsigned long color = inString.toInt();
        byte mode = (color >> 24) & 0xff;  // high-order (leftmost) byte: bits 24-31

        // Modes
        switch(mode){
          case 0: // Turn OFF lights
              // Fade Out - Brightness
              setBrightness(0);
          break;

          case 1: // Turn ON lights
              // Fade In - Brightness
              setBrightness(brightness_desired);
          break;

          case 2: // Turn ON lights (With Color Transition)
              // Set Color
              setColors(color);
              // Fade In - Brightness
              setBrightness(brightness_desired);
          break;
          
          case 5: // Set COLORS
              // Set Color
              setColors(color);
          break;

          case 10: // Set Brightness
              brightness_desired = color & 0xff;
              setBrightness(brightness_desired);
          break;

          case 11: // Calibrate COLORs (Set Max White Point)
              r_maxwhite = (color >> 16) & 0xff;  // next byte, counting from left: bits 16-23
              g_maxwhite = (color >> 8) & 0xff;  // next byte, bits 8-15
              b_maxwhite = color & 0xff;  // low-order byte: bits 0-7
          break;

          case 20: // Enable Fading Transitions
              color_fade_step_total = COLOR_FADESTEPS_DEFAULT;
          break;

          case 21: // Disable Fading Transitions
              color_fade_step_total = 1;
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
    
  }else if(color_fade_step == 0){ 
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
  analogWrite(REDPIN,   filterBrightness(filterMaxWhite(r, r_maxwhite)));
  analogWrite(GREENPIN, filterBrightness(filterMaxWhite(g, g_maxwhite)));
  analogWrite(BLUEPIN,  filterBrightness(filterMaxWhite(b, b_maxwhite)));
}
