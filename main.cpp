#include <MPU6050.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <string.h>



#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128
#define MAX_SETS 3
#define OLED_ADDR 0x3C
#define MPU6050_ADDR 0x68

Adafruit_SSD1306 display(128, 64, &Wire, OLED_ADDR);





/////////////////(GLOBAL VARIABLES)///////////////////////////


// General Varibales 
bool restart = true;
bool start_process = false;
const byte sensorPin = A0;
const byte ButtonPin = 2;
const byte LED_red   = 4;
const byte LED_green = 5;
const byte RepPin = 6;
const byte ControlPin = 7;
byte reps = 0;
byte sets = 1;




// MPU6050 instance
MPU6050 mpu;




/////////////////////(FUNCTION DECLARATION)//////////////////
void displayStart();
void initdisplay(float textSize);
bool isButtonPressed(byte buttonPin);
void blinkLED(byte LED, unsigned long blinkInterval);
float getAcceleration(char axis);
bool detectRepetition(char axis);
void displayRepetitions(int repetitions);
void displayMovement(void);
void displayArrow(const char direction[]);

//////////////////////////////////////////////////////////


void setup() {
  Serial.begin(9600);

  Wire.begin(); // Initialize the Wire library for MPU6050
  // Initialize the MPU6050
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
 
  mpu.initialize();
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  mpu.setDLPFMode(MPU6050_DLPF_BW_20); 

  pinMode(LED_green, OUTPUT);
  pinMode(LED_red, OUTPUT);
  pinMode(ButtonPin, INPUT_PULLUP);
  
}


///////////////////(MAIN CODE)///////////////////////////////
// ...



void loop() {
  // Check if the restart flag is set and display the start screen
  if (restart) {
    displayStart();
  }
  restart = false;

  // Check if the button is pressed to start the process
  if (isButtonPressed(ButtonPin)) {
    start_process = true;
    display.clearDisplay();
    display.display();
  } 

  // Main loop to show the movement
  while (start_process) {
    // Blink the green LED
    blinkLED(LED_green, 200);
    
    // since the display functions dont clear the screens
    display.clearDisplay();

    // Display the arrow based on movement and the reps:
    displayMovement();
    
    // Detect the repetition on the y-axis and increment the repetition count
    if (detectRepetition('y')) {
      reps++;
    }

    // Check if the button is pressed to stop the process
    if (isButtonPressed(ButtonPin)) {
      digitalWrite(LED_green, LOW);
      start_process = false;
      restart = true;
      reps = 0;
    }
  }
}

// ...



//////////////////////////////////////(FUNCTION DEFINITIONS)///////////////////////////////////////////
//______________________________________________________________________________________________________________

void displayStart(){
  initdisplay(2.5);
  display.println("Welcome");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("Running Version:");
  display.print("1.1.2");
  display.display();
}

//______________________________________________________________________________________________________________

void initdisplay(float textSize) {
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
}

//______________________________________________________________________________________________________________

bool isButtonPressed(byte buttonPin) {
  static bool previousButtonState = HIGH;  // Initialize with HIGH (not pressed)
  bool currentButtonState = digitalRead(buttonPin);


  if (previousButtonState == HIGH && currentButtonState == LOW) {
    previousButtonState = currentButtonState;
    delay(50);  // Debounce delay
    return true;
  }


  previousButtonState = currentButtonState;
  return false;
}

//______________________________________________________________________________________________________________

void blinkLED(byte LED, unsigned long blinkInterval) {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  static bool ledState = false;


  if (currentMillis - previousMillis >= blinkInterval) {
    ledState = !ledState;
    digitalWrite(LED, ledState);


    previousMillis = currentMillis;
  }
}

//______________________________________________________________________________________________________________

float getAcceleration(char axis) {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Apply moving average filter to acceleration
  const int numSamples = 5;
  static int16_t samples[numSamples];
  static int sampleIndex = 0;

  int16_t axisAcceleration;

  switch (axis) {
    case 'x':
    case 'X':
      axisAcceleration = ax;
      break;

    case 'y':
    case 'Y':
      axisAcceleration = ay;
      break;

    case 'z':
    case 'Z':
      axisAcceleration = az;
      break;

    default:
      axisAcceleration = 0;
      break;
  }

  samples[sampleIndex] = axisAcceleration;
  sampleIndex = (sampleIndex + 1) % numSamples;

  float avgAcceleration = 0;
  for (int i = 0; i < numSamples; i++) {
    avgAcceleration += samples[i];
  }
  avgAcceleration /= numSamples;

  return avgAcceleration;
}

//______________________________________________________________________________________________________________

bool detectRepetition(char axis) {

  //Variables
  float startAcceleration = 0;
  float endAcceleration;
  float threshold = 0.2; // Adjust the threshold value as per sensitivity

  // Get the average acceleration for the specified axis
  float acceleration = getAcceleration(axis);

  // Check if the acceleration is above the threshold
  if (acceleration > startAcceleration + threshold) {
    // Device moved upwards, store the start acceleration
    startAcceleration = acceleration;
  } else if (acceleration < endAcceleration - threshold) {
    // Device moved downwards, store the end acceleration
    endAcceleration = acceleration;

    // Check if the difference between start and end acceleration is within a tolerance
    float accelerationDiff = startAcceleration - endAcceleration;
    if (abs(accelerationDiff) < threshold) {
      // Repetition detected
      return true;
    }
  }

  return false;
}

//______________________________________________________________________________________________________________

void displayRepetitions(int repetitions) {

  // Set text properties
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Display repetitions
  display.setCursor(0, 0);
  display.print("Reps: ");
  display.setCursor(0, 0 + 20);
  display.print(repetitions);

  // Update the display
  display.display();
}

//______________________________________________________________________________________________________________

void displayMovement() {
  float avgAcceleration = getAcceleration('y');
  // Determine the arrow direction based on average acceleration
  int arrowDirection;
  if (avgAcceleration > 0) {
    arrowDirection = 1; // Arrow Up
  } else {
    arrowDirection = -1; // Arrow Down
  }

  // Display arrow based on direction
  if (arrowDirection == 1) {
    displayArrow("Up");
  } else {
    displayArrow("Down");
  }

  displayRepetitions(reps);
  // Update the display
  display.display();
  
  delay(50);
}

//______________________________________________________________________________________________________________


void displayArrow(const char direction[]) {
  int centerX = SCREEN_WIDTH - 10;
  int centerY = SCREEN_HEIGHT / 2;
  int arrowSize = 24;
  int lineWidth = 10; // Adjust the line width as desired
  
  if (strcmp(direction, "Down") == 0) {
    display.drawLine(centerX, centerY - arrowSize, centerX - lineWidth, centerY + arrowSize, SSD1306_WHITE);
    display.drawLine(centerX, centerY - arrowSize, centerX + lineWidth, centerY + arrowSize, SSD1306_WHITE);
    display.drawLine(centerX - lineWidth, centerY + arrowSize, centerX + lineWidth, centerY + arrowSize, SSD1306_WHITE);
  }
  else if (strcmp(direction, "Up") == 0) {
    display.drawLine(centerX, centerY + arrowSize, centerX - lineWidth, centerY - arrowSize, SSD1306_WHITE);
    display.drawLine(centerX, centerY + arrowSize, centerX + lineWidth, centerY - arrowSize, SSD1306_WHITE);
    display.drawLine(centerX - lineWidth, centerY - arrowSize, centerX + lineWidth, centerY - arrowSize, SSD1306_WHITE);
  }
}

//______________________________________________________________________________________________________________