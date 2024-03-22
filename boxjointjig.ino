#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include <Stepper.h>

// Define used pins
#define ENCODER_DT 3
#define ENCODER_CLK 2
#define ENCODER_SW 4

#define LEFT_LIMIT_SW 5
#define RIGHT_LIMIT_SW 6

#define MOTOR_STEP 12
#define MOTOR_DIR 11

const int stepsPerRevolution = 200; 

// Create instances for the used devices
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(ENCODER_DT, ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);
Stepper motor(stepsPerRevolution, MOTOR_STEP, MOTOR_DIR);

// This is for the neater console prints
template <typename T>
Print& operator<<(Print& printer, T value)
{
  printer.print(value);
  return printer;
}

float version = 0.1;

int current_hole = 1;
int hole_steps = 0;
int blade_steps = 0;
int position = 0;
int end_position = 0;
bool hole_cutted = false;

int menu = 1;
int current_menu = 1;
int main_menu = 1;
int setup_menu = 2;
int cutting_menu = 3;
int board_width_menu = 4;
int pins_count_menu = 5;
int start_pins_menu = 6;
int blade_width_menu = 7;
int home_position_menu = 8;

float thread_pitch = 1.25; // M8 thread as default
float blade_width = 3.4; // Your default saw blade width in mm
int board_width = 100; // default board width in mm
int pin_count = 5; // default pin count
int overlap = 4; // Blade overlap from previous cut
String start_pins = "yes";

bool input_mode = false;
bool increase = false;
bool now_cutting = false;
int value = 0;

//int Testled = 13;

int currentStateCLK;
int previousStateCLK; 

bool buttonPressed = false;
bool homing = false;
bool homing_done = false;
bool start_cutting = false;

void checkButton()
{
    if (!digitalRead(ENCODER_SW)){
      Serial << "Button pressed" << '\n';
      if (now_cutting){
        calculate_next_position();
      }
      else {
        switchMenu();
        updateMenu();
      }
      delay(100);
      while (!digitalRead(ENCODER_SW));
    }
}

void checkLimitSwitch() {
  if (homing) {
    moveHomePosition();
  }
  if (current_menu == home_position_menu and homing_done) {
    ready_to_cut();    
  }
}

void welcomeMenu() {
  lcd.clear();
  lcd.print("BoxJointJig v");
  lcd.setCursor(13, 0);
  lcd.print(version);
  lcd.setCursor(0, 1);
  lcd.print("by Mika Naumi");
  delay(1500);
  updateMainMenu();
}

void switchMenu() {
  switch (current_menu) {
    case 1:
      switch (menu) {
        case 1:
          current_menu = setup_menu;
          menu = 1;
          break;
        case 2:
          current_menu = cutting_menu;
          break;
      }
      break;
    case 2:
      switch (menu) {
        //case 0:
        //  menu = 1;
        //  break;
        case 1:
          current_menu = board_width_menu;
          menu = 1;
          break;
        case 2:
          current_menu = pins_count_menu;
          menu = 1;
          break;
        case 3:
          current_menu = start_pins_menu;
          menu = 1;
          break;
        case 4:
          current_menu = blade_width_menu;
          menu = 1;
          break;
        case 5:
          current_menu = main_menu;
          menu = 1;
          break;
      }
      break;
    case 3: // cutting_menu
      current_menu = home_position_menu;
      lcd.clear();
      lcd.print("Homing...");
      homing = true;
      break;
    case 4: // board_width_menu
      current_menu = setup_menu;
      menu = 1;
      break;
    case 5: // pins_count_menu
      current_menu = setup_menu;
      menu = 2;
      break;
    case 6: // start_pins_menu
      current_menu = setup_menu;
      menu = 3;
      break;
    case 7: // blade_width_menu
      current_menu = setup_menu;
      menu = 4;
      break;
  }
}

void updateMenu() {
  switch (current_menu) {
    case 1:
      updateMainMenu();
      break;
    case 2:
      updateSetupMenu();
      break;
    case 3:
      updateCuttingMenu();
      break;
    case 4:
      inputBoardWidthMenu();
      break;
    case 5:
      inputPinsCountMenu();
      break;
    case 6:
      inputStartPinsMenu();
      break;
  }
}

void updateMainMenu() {
  switch (menu) {
    case 0:
      menu = 1;
      break;
    case 1:
      lcd.clear();
      lcd.print(">Setup");
      lcd.setCursor(0, 1);
      lcd.print(" Start Cutting");
      break;
    default:
      lcd.clear();
      lcd.print(" Setup");
      lcd.setCursor(0, 1);
      lcd.print(">Start Cutting");
      menu = 2;
      break;
  }
}

void updateSetupMenu() {
  switch (menu) {
    case 0:
      menu = 1;
      break;
    case 1:
      lcd.clear();
      lcd.print(">Board Width:");
      lcd.setCursor(13, 0);
      lcd.print(board_width);
      lcd.setCursor(0, 1);
      lcd.print(" Nbr of Pins:");
      lcd.setCursor(13, 1);
      lcd.print(pin_count);
      break;
    case 2:
      lcd.clear();
      lcd.print(" Board Width:");
      lcd.setCursor(13, 0);
      lcd.print(board_width);
      lcd.setCursor(0, 1);
      lcd.print(">Nbr of Pins:");
      lcd.setCursor(13, 1);
      lcd.print(pin_count);
      break;
    case 3:
      lcd.clear();
      lcd.print(" Nbr of Pins:");
      lcd.setCursor(13, 0);
      lcd.print(pin_count);
      lcd.setCursor(0, 1);
      lcd.print(">Start Pins:");
      lcd.setCursor(12, 1);
      lcd.print(start_pins);
      break;
    case 4:
      lcd.clear();
      lcd.print(" Start Pins");
      lcd.setCursor(12, 0);
      lcd.print(start_pins);
      lcd.setCursor(0, 1);
      lcd.print(">Blade Width:");
      lcd.setCursor(13, 1);
      lcd.print(blade_width);
      break;
    case 5:
      lcd.clear();
      lcd.print(" Blade Width:");
      lcd.setCursor(13, 0);
      lcd.print(blade_width);
      lcd.setCursor(0, 1);
      lcd.print(">Back");
      break;
    default:
      menu = 5;
      break;
  }
}

void inputBoardWidthMenu() {
      lcd.clear();
      lcd.print("Board Width");
      lcd.setCursor(0, 1);
      if (increase and input_mode){
        board_width++;
      }
      else if (input_mode){
        board_width--;
      }
      lcd.print(board_width);
      input_mode = true;
}

void inputPinsCountMenu() {
      lcd.clear();
      lcd.print("Nbr of Pins");
      lcd.setCursor(0, 1);
      if (increase and input_mode){
        pin_count++;
      }
      else if (input_mode){
        pin_count--;
      }
      lcd.print(pin_count);
      input_mode = true;
}

void inputStartPinsMenu() {
      lcd.clear();
      lcd.print("Start Pins");
      lcd.setCursor(0, 1);
      if (increase and input_mode){
        start_pins = "no";
      }
      else if (input_mode){
        start_pins = "yes";
      }
      lcd.print(start_pins);
      input_mode = true;
}

void updateCuttingMenu() {
  lcd.clear();
  lcd.print("Press to go HOME");
  lcd.setCursor(0, 1);
  lcd.print("position");
}

void moveHomePosition() {
    motor.setSpeed(600);
    if (!digitalRead(RIGHT_LIMIT_SW)){
      //Serial.println("Limit reached");
      Serial << "Limit reached" << '\n';
      homing = false;
      //lcd.clear();
      //lcd.print("Now in HOME");
      //lcd.setCursor(0, 1);
      //lcd.print("position");
      //switchMenu();
      //updateMenu();
      //delay(100);
      ready_to_cut();
      while (!digitalRead(RIGHT_LIMIT_SW));
    }
    else {
      motor.step(3);
    }
}

// Maybe this is not necessary
void fine_tune_homing() {
    motor.setSpeed(10);
    if (digitalRead(RIGHT_LIMIT_SW)){
      Serial.println("Fine tune reaced");
      homing_done = true;
      while (digitalRead(RIGHT_LIMIT_SW));
    }
    else {
      motor.step(-1);
    }
}

void ready_to_cut() {
  // move to first cut position
  int pin_size = board_width/pin_count/2;
  Serial << "pin size (mm) is: " << pin_size << '\n';
  hole_steps = (pin_size/thread_pitch)*stepsPerRevolution; // Step count for hole
  Serial << "hole size in steps: " << hole_steps << '\n';
  blade_steps = ((blade_width/thread_pitch)*stepsPerRevolution) - overlap;
  Serial << "blade size in steps: " << blade_steps << '\n';
  if (start_pins == "yes") {
    position = hole_steps;
  }
  end_position = position + hole_steps;
  Serial << "end position in steps: " << end_position << '\n';
  motor.setSpeed(200);
  motor.step(-position);
  cutting_screen();
}

void cutting_screen() {
  now_cutting = true;
  Serial << "Position: " << position << '\n';
  lcd.clear();
  lcd.print("Now cutting ");
  lcd.print(current_hole);
  lcd.print("/");
  lcd.print(pin_count);
}

void calculate_next_position() {
  // If current hole was complitely cut, move to the next hole
  if (hole_cutted) {
    position = position + hole_steps;
    end_position = position + hole_steps + blade_steps;
    Serial << "end position in steps: " << end_position << '\n';
    if (current_hole == pin_count)
      lcd.clear();
      lcd.print("All done!");
    current_hole++;
    hole_cutted = false;
  }
  // check if this is the last cut for the current hole
  if ((position + blade_steps + blade_steps) >= end_position) {
    position = end_position - blade_steps;
    hole_cutted = true;
  }
  else {
    position = position + blade_steps;
  }
  motor.setSpeed(200);
  motor.step(-position);
  cutting_screen();
}

void setup() {
  Wire.begin();

  lcd.begin(16,2);
  lcd.backlight();

  // Setup Serial Monitor
  Serial.begin (9600);
  encoder.setPosition(0);

  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(LEFT_LIMIT_SW, INPUT_PULLUP);
  pinMode(RIGHT_LIMIT_SW, INPUT_PULLUP);

  welcomeMenu();
}

void loop() {
    checkLimitSwitch();

    encoder.tick();
    int newPosition = encoder.getPosition();

    if (newPosition > 0) {
      if (input_mode) {
        increase = true;
      }
      menu++;
      encoder.setPosition(0);
      Serial << menu;
      updateMenu();
    }
    else if (newPosition < 0) {
      if (input_mode) {
        increase = false;
      }
      menu--;
      encoder.setPosition(0);
      Serial << menu;
      updateMenu();
    }
    checkButton();
}

