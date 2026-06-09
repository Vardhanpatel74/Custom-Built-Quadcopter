#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <math.h>

// --- HARDWARE SETUP ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5}; 
byte colPins[COLS] = {6, 7, 8, 9}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- STATE MACHINE ---
enum Mode { BASIC, LOG, EQUATION, TRIG, QUAD, POWER, SYS3 };
Mode currentMode = BASIC;

// --- SHARED VARIABLES ---
String inputString = "";
bool returnToStart = false; // Flag for the Long-Press 'A' lock screen

// Basic Mode Variables
float num1 = 0;
float num2 = 0;
char action = ' ';
bool newCalculation = true;

// 2-Var Equation Variables
float eq[6]; 
int eqIndex = 0;
String prompts[] = {"a1: ", "b1: ", "c1: ", "a2: ", "b2: ", "c2: "};

// 3-Var Equation Variables
float eq3[12]; 
int eq3Index = 0;
String prompts3[] = {"a1:", "b1:", "c1:", "d1:", "a2:", "b2:", "c2:", "d2:", "a3:", "b3:", "c3:", "d3:"};

// Quadratic Mode Variables
float quadEq[3];
int quadIndex = 0;
String quadPrompts[] = {"a: ", "b: ", "c: "};

// Power Mode Variables
float powerBase = 0;
int powerState = 0;

// --- SETUP ---
void setup() {
  lcd.init();
  lcd.backlight();
  
  keypad.setHoldTime(3000);
  keypad.addEventListener(keypadEvent); 
  
  runTutorial();
  resetToBasic();
}

// --- STARTUP INSTRUCTIONS (TUTORIAL) ---
void runTutorial() {
  lcd.clear();
  lcd.print("Welcome to Calc!");
  lcd.setCursor(0, 1);
  lcd.print("Press # for next");
  while(keypad.waitForKey() != '#');

  lcd.clear();
  lcd.print("A:+ B:- C:x D:/");
  lcd.setCursor(0, 1);
  lcd.print("Press * for next");
  while(keypad.waitForKey() != '*');

  lcd.clear();
  lcd.print("Hold A: Basic");
  lcd.setCursor(0, 1);
  lcd.print("Hold B: Trig");
  while(keypad.waitForKey() != '*');

  lcd.clear();
  lcd.print("Hold C: Quad Fn.");
  lcd.setCursor(0, 1);
  lcd.print("Hold D: Root/Pwr");
  while(keypad.waitForKey() != '*');

  lcd.clear();
  lcd.print("Hold #: Log");
  lcd.setCursor(0, 1);
  lcd.print("Hold *: Lin. Eq.");
  while(keypad.waitForKey() != '*');

  lcd.clear();
  lcd.print("Press * to start");
  lcd.setCursor(0, 1);
  lcd.print("the work");
  while(keypad.waitForKey() != '*');
}

// --- MAIN LOOP ---
void loop() {
  // Check if user triggered the "Lock Screen" by holding A
  if (returnToStart) {
    lcd.clear();
    lcd.print("Press * to start");
    lcd.setCursor(0, 1);
    lcd.print("the work");
    while(keypad.waitForKey() != '*'); // Freeze until * is pressed
    returnToStart = false;
    resetToBasic();
    return;
  }

  char key = keypad.getKey();

  if (key && keypad.getState() == PRESSED) {
    
    // GLOBAL ESCAPE HATCH: Short press A in any mode (except Basic) goes to Basic
    if (key == 'A' && currentMode != BASIC) {
      resetToBasic();
      return;
    }

    switch (currentMode) {
      case BASIC: handleBasicMode(key); break;
      case LOG: handleLogMode(key); break;
      case EQUATION: handleEquationMode(key); break;
      case TRIG: handleTrigMode(key); break;
      case QUAD: handleQuadMode(key); break;
      case POWER: handlePowerMode(key); break;
      case SYS3: handleSys3Mode(key); break;
    }
  }
}

// --- EVENT LISTENER FOR LONG PRESSES ---
void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()) {
    case HOLD:
      if (key == 'A') {
        returnToStart = true; // Flags the main loop to show the start screen
      } 
      else if (key == 'B') {
        currentMode = TRIG;
        inputString = "";
        lcd.clear(); lcd.print("Trig Mode"); delay(1500);
        lcd.clear(); lcd.print("Angle (Deg):"); lcd.setCursor(0, 1);
      }
      else if (key == 'C') {
        currentMode = QUAD;
        quadIndex = 0; inputString = "";
        lcd.clear(); lcd.print("Quadratic Mode"); delay(1500);
        promptQuad();
      }
      else if (key == 'D') {
        currentMode = POWER;
        inputString = ""; powerState = 0;
        lcd.clear(); lcd.print("Power/Root Mode"); delay(1500);
        lcd.clear(); lcd.print("Val:"); lcd.setCursor(0, 1);
      }
      else if (key == '*') {
        currentMode = EQUATION;
        eqIndex = 0; inputString = "";
        lcd.clear(); lcd.print("2-Var Eq Mode"); delay(1500);
        promptEquation();
      } 
      else if (key == '#') {
        currentMode = LOG;
        inputString = "";
        lcd.clear(); lcd.print("Logarithm Mode"); delay(1500);
        lcd.clear(); lcd.print("Log10(X):"); lcd.setCursor(0, 1);
      }
      else if (key == '3') {
        currentMode = SYS3;
        eq3Index = 0; inputString = "";
        lcd.clear(); lcd.print("3-Var Eq Mode"); delay(1500);
        promptSys3();
      }
      break;
  }
}

// --- MODE 1: BASIC CALCULATOR ---
void handleBasicMode(char key) {
  if (newCalculation) {
    lcd.clear();
    num1 = 0; num2 = 0; action = ' '; inputString = "";
    newCalculation = false;
  }

  if (key >= '0' && key <= '9') {
    inputString += key;
    if (action == ' ') {
      lcd.setCursor(0, 0); lcd.print(inputString);
    } else {
      lcd.setCursor(0, 1); lcd.print(inputString);
    }
  } 
  else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
    if (action == ' ' && inputString.length() > 0) {
      num1 = inputString.toFloat();
      inputString = "";
      action = key;
      
      lcd.setCursor(15, 0);
      if (key == 'A') lcd.print('+');
      if (key == 'B') lcd.print('-');
      if (key == 'C') lcd.print('*');
      if (key == 'D') lcd.print('/');
    }
  } 
  else if (key == '#') { 
    if (inputString.length() > 0) {
      num2 = inputString.toFloat();
      inputString = "";
      calculateBasicResult();
      newCalculation = true;
    }
  } 
  else if (key == '*') { 
    resetToBasic();
  }
}

void calculateBasicResult() {
  float total = 0;
  switch (action) {
    case 'A': total = num1 + num2; break; 
    case 'B': total = num1 - num2; break; 
    case 'C': total = num1 * num2; break; 
    case 'D': 
      if (num2 != 0) total = num1 / num2; 
      else { lcd.clear(); lcd.print("Error: Div by 0"); delay(2000); return; }
      break;
  }
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Result:");
  lcd.setCursor(0, 1); lcd.print(total);
}

// --- MODE 2: LOGARITHMS ---
void handleLogMode(char key) {
  if (key >= '0' && key <= '9') {
    inputString += key;
    lcd.print(key);
  } 
  else if (key == '#') { 
    if (inputString.length() > 0) {
      float val = inputString.toFloat();
      lcd.clear();
      lcd.print("log("); lcd.print(val, 0); lcd.print(") =");
      lcd.setCursor(0, 1); lcd.print(log10(val));
      inputString = ""; 
    }
  } 
  else if (key == '*') { 
    inputString = "";
    lcd.clear(); lcd.print("Log10(X):"); lcd.setCursor(0, 1);
  }
}

// --- MODE 3: 2-VARIABLE EQUATIONS ---
void handleEquationMode(char key) {
  if (key >= '0' && key <= '9') {
    inputString += key;
    promptEquation();
  } 
  else if (key == 'B') { 
    if (inputString.startsWith("-")) inputString.remove(0, 1);
    else inputString = "-" + inputString;
    promptEquation();
  }
  else if (key == '#') { 
    if (inputString.length() > 0 || inputString == "-") {
      if (inputString == "-") inputString = "-1"; 
      eq[eqIndex] = inputString.toFloat();
      inputString = "";
      eqIndex++;
      if (eqIndex > 5) solveEquations();
      else promptEquation();
    }
  } 
  else if (key == '*') { 
    if (inputString.length() > 0) { inputString = ""; promptEquation(); } 
    else { eqIndex = 0; promptEquation(); }
  }
}

void promptEquation() {
  lcd.clear();
  lcd.print(prompts[eqIndex]); lcd.print(inputString);
}

void solveEquations() {
  float D = (eq[0] * eq[4]) - (eq[3] * eq[1]);
  lcd.clear();
  if (D == 0) {
    lcd.print("No Unique Sol.");
  } else {
    float x = ((eq[2] * eq[4]) - (eq[5] * eq[1])) / D;
    float y = ((eq[0] * eq[5]) - (eq[3] * eq[2])) / D;
    lcd.print("X1 = "); lcd.print(x);
    lcd.setCursor(0, 1); lcd.print("X2 = "); lcd.print(y);
  }
  eqIndex = 0; 
}

// --- MODE 4: TRIGONOMETRY ---
void handleTrigMode(char key) {
  if (key >= '0' && key <= '9') {
    inputString += key;
    lcd.print(key);
  } 
  // Changed to 1, 2, 3 so 'A' can be used to escape
  else if (key == '1' || key == '2' || key == '3') {
    if (inputString.length() > 0) {
      float val = inputString.toFloat();
      float rad = val * (PI / 180.0);
      lcd.clear();
      if (key == '1') { lcd.print("sin("); lcd.print(val, 0); lcd.print(")="); lcd.setCursor(0, 1); lcd.print(sin(rad)); }
      if (key == '2') { lcd.print("cos("); lcd.print(val, 0); lcd.print(")="); lcd.setCursor(0, 1); lcd.print(cos(rad)); }
      if (key == '3') { lcd.print("tan("); lcd.print(val, 0); lcd.print(")="); lcd.setCursor(0, 1); lcd.print(tan(rad)); }
      inputString = "";
    }
  } 
  else if (key == '*') {
    inputString = "";
    lcd.clear(); lcd.print("Angle (Deg):"); lcd.setCursor(0, 1);
  }
}

// --- MODE 5: QUADRATIC EQUATIONS ---
void handleQuadMode(char key) {
  if (key >= '0' && key <= '9') {
    inputString += key;
    promptQuad();
  } 
  else if (key == 'B') { 
    if (inputString.startsWith("-")) inputString.remove(0, 1);
    else inputString = "-" + inputString;
    promptQuad();
  }
  else if (key == '#') { 
    if (inputString.length() > 0 || inputString == "-") {
      if (inputString == "-") inputString = "-1"; 
      quadEq[quadIndex] = inputString.toFloat();
      inputString = "";
      quadIndex++;
      if (quadIndex > 2) solveQuad();
      else promptQuad();
    }
  } 
  else if (key == '*') { 
    if (inputString.length() > 0) { inputString = ""; promptQuad(); } 
    else { quadIndex = 0; promptQuad(); }
  }
}

void promptQuad() {
  lcd.clear();
  lcd.print(quadPrompts[quadIndex]); lcd.print(inputString);
}

void solveQuad() {
  float a = quadEq[0], b = quadEq[1], c = quadEq[2];
  float d = (b * b) - (4 * a * c); 
  
  lcd.clear();
  if (a == 0) {
    lcd.print("Not Quadratic");
  } else if (d < 0) {
    lcd.print("Imaginary Roots");
  } else {
    float x1 = (-b + sqrt(d)) / (2 * a);
    float x2 = (-b - sqrt(d)) / (2 * a);
    lcd.print("X1 = "); lcd.print(x1);
    lcd.setCursor(0, 1); lcd.print("X2 = "); lcd.print(x2);
  }
  quadIndex = 0; 
}

// --- MODE 6: POWERS & ROOTS ---
void handlePowerMode(char key) {
  if (key >= '0' && key <= '9') {
    inputString += key;
    lcd.print(key);
  } 
  // Changed to 1 and 2 so 'A' can be used to escape
  else if (key == '1') { 
    if (inputString.length() > 0) {
      float val = inputString.toFloat();
      lcd.clear(); lcd.print("sqrt("); lcd.print(val, 0); lcd.print(")=");
      lcd.setCursor(0, 1); lcd.print(sqrt(val));
      inputString = ""; powerState = 0;
    }
  }
  else if (key == '2') { 
    if (inputString.length() > 0) {
      powerBase = inputString.toFloat();
      inputString = "";
      powerState = 1; 
      lcd.print("^");
    }
  }
  else if (key == '#') { 
    if (powerState == 1 && inputString.length() > 0) {
      float exp = inputString.toFloat();
      lcd.clear(); lcd.print(powerBase, 0); lcd.print("^"); lcd.print(exp, 0); lcd.print("=");
      lcd.setCursor(0, 1); lcd.print(pow(powerBase, exp));
      inputString = ""; powerState = 0;
    }
  }
  else if (key == '*') {
    inputString = ""; powerState = 0;
    lcd.clear(); lcd.print("Val:"); lcd.setCursor(0, 1);
  }
}

// --- MODE 7: 3-VARIABLE EQUATIONS ---
void handleSys3Mode(char key) {
  if (key >= '0' && key <= '9') {
    inputString += key;
    promptSys3();
  } 
  else if (key == 'B') { 
    if (inputString.startsWith("-")) inputString.remove(0, 1);
    else inputString = "-" + inputString;
    promptSys3();
  }
  else if (key == '#') { 
    if (inputString.length() > 0 || inputString == "-") {
      if (inputString == "-") inputString = "-1"; 
      eq3[eq3Index] = inputString.toFloat();
      inputString = "";
      eq3Index++;
      if (eq3Index > 11) solveSys3();
      else promptSys3();
    }
  } 
  else if (key == '*') { 
    if (inputString.length() > 0) { inputString = ""; promptSys3(); } 
    else { eq3Index = 0; promptSys3(); }
  }
}

void promptSys3() {
  lcd.clear();
  lcd.print(prompts3[eq3Index]); lcd.print(inputString);
}

float det3(float a1, float b1, float c1, float a2, float b2, float c2, float a3, float b3, float c3) {
  return a1*(b2*c3 - b3*c2) - b1*(a2*c3 - a3*c2) + c1*(a2*b3 - a3*b2);
}

void solveSys3() {
  float D = det3(eq3[0], eq3[1], eq3[2], eq3[4], eq3[5], eq3[6], eq3[8], eq3[9], eq3[10]);
  
  lcd.clear();
  if (D == 0) {
    lcd.print("No Unique Sol.");
    delay(2000);
  } else {
    float Dx = det3(eq3[3], eq3[1], eq3[2], eq3[7], eq3[5], eq3[6], eq3[11], eq3[9], eq3[10]);
    float Dy = det3(eq3[0], eq3[3], eq3[2], eq3[4], eq3[7], eq3[6], eq3[8], eq3[11], eq3[10]);
    float Dz = det3(eq3[0], eq3[1], eq3[3], eq3[4], eq3[5], eq3[7], eq3[8], eq3[9], eq3[11]);
    
    float x1 = Dx / D;
    float x2 = Dy / D;
    float x3 = Dz / D;
    
    lcd.print("X1 = "); lcd.print(x1);
    lcd.setCursor(0, 1); 
    lcd.print("X2 = "); lcd.print(x2);
    
    keypad.waitForKey(); 
    
    lcd.clear();
    lcd.print("X3 = "); lcd.print(x3);
    lcd.setCursor(0, 1);
    lcd.print("Press Any Key...");
    
    keypad.waitForKey(); 
  }
  eq3Index = 0; 
  promptSys3(); 
}

// --- UTILITY FUNCTIONS ---
void resetToBasic() {
  currentMode = BASIC;
  inputString = "";
  num1 = 0; num2 = 0; action = ' ';
  newCalculation = true;
  lcd.clear();
  lcd.print("Basic Mode");
  delay(1000);
  lcd.clear();
  lcd.print("Ready");
}