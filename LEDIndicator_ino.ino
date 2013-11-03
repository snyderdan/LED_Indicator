

// I/O Pin setup
// Used for communication between cRio and Arduino
# define BIT1 2
# define BIT2 3
# define BIT3 4

// PWM ports act as analog outputs for LEDs
// Ports 5 and 6 are known to have unexpected duty cycles
// So we'll just go with the upper 3 PWM ports
# define PORT_RED   9
# define PORT_GREEN 10
# define PORT_BLUE  11

// Analog input pins
# define RANDSOURCE 1

// Color map
# define ID_OFF    0b111
# define ID_RED    0b001
# define ID_GREEN  0b010
# define ID_BLUE   0b011
# define ID_YELLOW 0b100
# define ID_ORANGE 0b101
# define ID_WHITE  0b110
# define ID_UNDEF1 0b000

# define COL_OFF    {0, 0, 0}
# define COL_RED    {255, 0, 0}
# define COL_GREEN  {0, 255, 0}
# define COL_BLUE   {0, 0, 255}
# define COL_YELLOW {200, 120, 10}
# define COL_ORANGE {255, 70, 12}
# define COL_WHITE  {255, 255, 255}
# define COL_UNDEF1 {0, 0, 0}

int color_map[8][3] = {COL_OFF, COL_RED, COL_GREEN, COL_BLUE, 
                       COL_YELLOW, COL_ORANGE, COL_WHITE, COL_UNDEF1 // COL_UNDEF1 is the target color for the disco mode
                     };

// vars for disco mode (all time in ms)
# define FADE_IN_STATE  0
# define CONSTANT_STATE 1
# define FADE_OUT_STATE 2
# define TIMEOUT_STATE  3
int disco_state = FADE_IN_STATE;

long disco_prev_time = 0; // Time of the previous loop
const long disco_fade_in  = 1000;
const long disco_constant = 2000;
const long disco_fade_out = 1000;
const long disco_timeout  = 500;
const long disco_cycle_time = disco_fade_in + disco_constant + disco_fade_out + disco_timeout;

float disco_color[3] = {0.0, 0.0, 0.0};
int disco_dest = ID_RED;
// vars to filter line between sidecar and arduino
int num_of_zeros = 0;
const int acceptable_loss = 10;

void write_color(int *col) {
//  Serial.print(col[0], DEC);
//  Serial.print(" ");
//  Serial.print(col[1], DEC);
//  Serial.print(" ");
//  Serial.println(col[2], DEC);
  analogWrite(PORT_RED, col[0]);
  analogWrite(PORT_GREEN, col[1]);
  analogWrite(PORT_BLUE, col[2]);
}

void test() {
  int i;
  //write_color(color_map[ID_WHITE]);
  //delay(5000);
  for (i=7; i>=0; i--) {
    write_color(color_map[i]);
    delay(1000);
  }
}

void setup() {
  /* 
   * Following I/O pins indicate the color to turn the LED's based on the following color map:
   * 001 = Red
   * 010 = Green
   * 011 = Blue
   * 100 = Yellow
   * 101 = Orange
   * 110 = White
   * 111 = Undefined
   *
   * Bits are BIT1, BIT2, BIT3
   */
  pinMode(BIT1, INPUT);
  pinMode(BIT2, INPUT);
  pinMode(BIT3, INPUT);
  
  analogWrite(PORT_RED, 0);
  analogWrite(PORT_GREEN, 0);
  analogWrite(PORT_BLUE, 0);
  
  test();
  
//  Serial.begin(9600);
}

void loop() {
  //int color_id = ID_UNDEF1, *output;
  int color_id = 0, *output;
//  color_id = (digitalRead(BIT1) << 2) | (digitalRead(BIT2) << 1) | digitalRead(BIT3); // Gets current color code
  if (digitalRead(BIT1)) color_id |= 1;
  if (digitalRead(BIT2)) color_id |= 2;
  if (digitalRead(BIT3)) color_id |= 4;
//  Serial.print(color_id, DEC);
//  Serial.print(" ");
  
  if (color_id == ID_OFF) { // If I happen to get a zero from the sidecar
    num_of_zeros++;         // Increment zero count
    if (num_of_zeros < acceptable_loss) return; // But if it's not more than the acceptable count then disregard the zero; assume its noise
  }
  num_of_zeros = 0; // If I am not getting a zero assume that it is the proper color and zero the counter
  
  if (color_id != ID_UNDEF1) {
    disco_dest = -1;
    output = color_map[color_id]; // Maps the color code to an output pin
    write_color(output);
  } else {
     float delta = ((float) (millis() - disco_prev_time))/disco_constant;
     disco_prev_time = millis();
//     Serial.print(disco_color[0], 3);
//     Serial.print(" ");
//     Serial.print(disco_color[1], 3);
//     Serial.print(" ");
//     Serial.print(disco_color[2], 3);
//     Serial.print(" ");
     
     if(disco_dest == ID_RED){
        if(disco_color[0] < 1){
          disco_color[0] +=delta; 
          disco_color[2] -=delta;
        }else{
          disco_color[0] = 1;
          disco_color[2] = 0;
          disco_dest = ID_GREEN;
        }
     }else if(disco_dest == ID_GREEN){
       if(disco_color[1] < 1){
          disco_color[1] +=delta; 
          disco_color[0] -=delta;
        }else{
          disco_color[1] = 1;
          disco_color[0] = 0;
          disco_dest = ID_BLUE;
        }
     }else if(disco_dest == ID_BLUE){
       if(disco_color[2] < 1){
          disco_color[2] +=delta; 
          disco_color[1] -=delta;
        }else{
          disco_color[2] = 1;
          disco_color[1] = 0;
          disco_dest = ID_RED;
        }
     }
     for(int i = 0; i < 3; i++){
       if(disco_color[i] < 0){
         disco_color[i] = 0;
       }
     }
     int current_color[3] = {(int) (disco_color[0]*255.0), (int) (disco_color[1]*255.0), (int) (disco_color[2]*255.0)};
     write_color(current_color);
  }
}
