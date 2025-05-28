// Croitoru Constantin-Bogdan
// grupa 334CA

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// LCD Nokia 5110
#define PIN_SCLK 7
#define PIN_DIN  6
#define PIN_DC   5
#define PIN_CS   4
#define PIN_RST  12 // initial 3, dar l-am schimbat ca sa-l folosesc la buzzer
Adafruit_PCD8544 display(PIN_SCLK, PIN_DIN, PIN_DC, PIN_CS, PIN_RST);

// Joystick
#define JOY_X_CHANNEL PC0 //A0
#define JOY_Y_CHANNEL PC1 //A1
#define JOY_BTN_BIT PB0  // D8

// Buzzer
#define BUZZER_PIN 3  // PWM cu timer2

// LED RGB
#define LED_R PB3 // D11
#define LED_G PB2 // D10
#define LED_B PB1 // D9

#define DEAD_ZONE 100

void setRGB(bool r, bool g, bool b) {
  if (r) 
    PORTB |= (1 << LED_R); 
  else 
    PORTB &= ~(1 << LED_R);

  if (g) 
    PORTB |= (1 << LED_G); 
  else 
    PORTB &= ~(1 << LED_G);

  if (b) 
    PORTB |= (1 << LED_B); 
  else 
    PORTB &= ~(1 << LED_B);
}

// pentru joc
char board[3][3];
int cursorX = 0;
int cursorY = 0;
bool turnX = true;
bool gameOver = false;
int scoreX = 0;
int scoreO = 0;

enum GameMode { MODE_AI, MODE_RANDOM };
GameMode gameMode = MODE_AI;

int readAnalog(uint8_t channel) {
  ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
  ADMUX |= (1 << REFS0); // AVcc
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

bool readJoystickButton() {
  return !(PINB & (1 << JOY_BTN_BIT));
}

// PWM control buzzer
void startPWM(uint16_t freqHz) {
  pinMode(BUZZER_PIN, OUTPUT);
  uint32_t top = (F_CPU / (2UL * 64UL * freqHz)) - 1;
  if (top > 255) 
    top = 255;  // OCR2A max = 255

  TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20); // Fast PWM, OC2B
  TCCR2B = (1 << WGM22) | (1 << CS22);                 // Prescaler = 64
  OCR2A = top;
  OCR2B = top / 2; // 50% duty cycle
}

void stopPWM() {
  TCCR2A = 0;
  TCCR2B = 0;
  digitalWrite(BUZZER_PIN, LOW);
}

void playTone(uint16_t freqHz, uint16_t durMs) {
  startPWM(freqHz);
  delay(durMs);
  stopPWM();
}

void winMelody() {
  playTone(262, 200); 
  delay(50);
  playTone(330, 200);
  delay(50);
  playTone(392, 200); 
  delay(50);
  playTone(523, 300); 
  delay(50);
}

void lossMelody() {
  playTone(392, 200); 
  delay(50);
  playTone(330, 200); 
  delay(50);
  playTone(262, 300); 
  delay(50);
}


// functie pentru afisare meniu-lui
void showMenu() {
  int selection = 0;
  bool selected = false;
  unsigned long lastMove = 0;

  while (!selected) {
    int y = readAnalog(JOY_Y_CHANNEL);
    if (millis() - lastMove > 200) {
      if (y < 512 - DEAD_ZONE && selection > 0) {
        selection = 0;
        lastMove = millis();
      } else if (y > 512 + DEAD_ZONE && selection < 1) {
        selection = 1;
        lastMove = millis();
      }
    }

    if (readJoystickButton()) {
      gameMode = (selection == 0) ? MODE_AI : MODE_RANDOM;
      selected = true;
      delay(500);
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Alege Mod Joc:");

    display.setCursor(0, 10);
    if (selection == 0) 
      display.print("> ");
    display.println("1. Contra AI");

    display.setCursor(0, 20);
    if (selection == 1)
      display.print("> ");
    display.println("2. Random O");
    display.display();
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Start joc X si 0");
  display.display();
  delay(1000);
}


// functia pentru resetarea scorului
bool resetScore() {
  int selection = 0;
  unsigned long lastMove = 0;

  while (true) {
    int y = readAnalog(JOY_Y_CHANNEL);
    if (millis() - lastMove > 200) {
      if (y < 512 - DEAD_ZONE && selection > 0) {
        selection = 0;
        lastMove = millis();
      } else if (y > 512 + DEAD_ZONE && selection < 1) {
        selection = 1;
        lastMove = millis();
      }
    }

    if (readJoystickButton()) {
      delay(300);
      return (selection == 0);
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Resetezi scorul?");

    display.setCursor(0, 17);
    if (selection == 0) 
      display.print("> ");
    display.println("Da");

    display.setCursor(0, 27);
    if (selection == 1) 
      display.print("> ");
    display.println("Nu");
    display.display();
  }
}

// resetare joc
void resetGame() {
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 3; x++)
      board[y][x] = ' ';
  cursorX = 0;
  cursorY = 0;
  turnX = true;
  gameOver = false;

  if (resetScore()) {
    scoreX = 0;
    scoreO = 0;
  }
  setRGB(0, 0, 0);
}

void drawBoard() {
  display.clearDisplay();

  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      int px = x * 15;
      int py = y * 15;
      display.drawRect(px, py, 14, 14, BLACK);

      if (board[y][x] == 'X' || board[y][x] == 'O') {
        display.setCursor(px + 4, py + 3);
        display.print(board[y][x]);

      } else if (x == cursorX && y == cursorY && !gameOver) {
        display.setCursor(px + 4, py + 3);
        display.print('X');
      }
    }
  }

  display.setCursor(50, 0);
  display.print(gameMode == MODE_AI ? "AI" : "Rnd");

  display.setCursor(50, 10);
  display.print("X:");
  display.print(scoreX);

  display.setCursor(50, 20);
  display.print("O:");
  display.print(scoreO);
  display.display();
}

bool checkWin(char player) {
  for (int i = 0; i < 3; i++) {
    if (board[i][0] == player && board[i][1] == player && board[i][2] == player)
     return true;
    if (board[0][i] == player && board[1][i] == player && board[2][i] == player)
     return true;
  }
  return (board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
         (board[0][2] == player && board[1][1] == player && board[2][0] == player);
}

bool isBoardFull() {
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 3; x++)
      if (board[y][x] == ' ')
       return false;
  return true;
}

int minimax(bool isMaximizing, int depth) {
  if (checkWin('O'))
    return 17 - depth;
  if (checkWin('X'))
   return -17 - depth;
  if (isBoardFull())
    return 0;

  if (isMaximizing) {
    int bestScore = -1000;
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        if (board[y][x] == ' ') {
          board[y][x] = 'O';
          int score = minimax(false, depth + 1);
          board[y][x] = ' ';
          bestScore = max(score, bestScore);
        }
      }
    }
    return bestScore;

  } else {
    int bestScore = 1000;
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        if (board[y][x] == ' ') {
          board[y][x] = 'X';
          int score = minimax(true, depth + 1);
          board[y][x] = ' ';
          bestScore = min(score, bestScore);
        }
      }
    }
    return bestScore;
  }
}

void moveAI() {
  int bestScore = -1000;
  int moveX = -1, moveY = -1;
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      if (board[y][x] == ' ') {
        board[y][x] = 'O';
        int score = minimax(false, 1);
        board[y][x] = ' ';
        if (score > bestScore) {
          bestScore = score;
          moveX = x;
          moveY = y;
        }
      }
    }
  }
  if (moveX != -1 && moveY != -1) {
    board[moveY][moveX] = 'O';
    setRGB(1, 0, 0);
  }
  delay(1000);
}

void moveRandom() {
  int empty[9][2];
  int count = 0;
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      if (board[y][x] == ' ') {
        empty[count][0] = y;
        empty[count][1] = x;
        count++;
      }
    }
  }
  
  if (count > 0) {
    int r = random(count);
    board[empty[r][0]][empty[r][1]] = 'O';
    setRGB(1, 0, 0);
  }
  delay(1000);
}

void setup() {
  Serial.begin(9600);

  // Joystick button
  DDRB &= ~(1 << JOY_BTN_BIT);
  PORTB |= (1 << JOY_BTN_BIT);

  // ADC
  ADCSRA |= (1 << ADEN);

  // LED RGB ca output
  DDRB |= (1 << PB1) | (1 << PB2) | (1 << PB3);

  setRGB(0, 0, 0);

  display.begin();
  display.setContrast(20);
  showMenu();
  resetGame();
}

void loop() {
  int x = readAnalog(JOY_X_CHANNEL);
  int y = readAnalog(JOY_Y_CHANNEL);
  bool btn = readJoystickButton();

  if (!gameOver && turnX) {
    setRGB(0, 0, 1);

    if (x < 512 - DEAD_ZONE && cursorX > 0)
      cursorX--;
    if (x > 512 + DEAD_ZONE && cursorX < 2)
      cursorX++;
    if (y < 512 - DEAD_ZONE && cursorY > 0)
      cursorY--;
    if (y > 512 + DEAD_ZONE && cursorY < 2)
      cursorY++;
    delay(150);

    if (btn && board[cursorY][cursorX] == ' ') {
      board[cursorY][cursorX] = 'X';
      turnX = false;
      if (checkWin('X')) {
        scoreX++;
        gameOver = true;
        setRGB(0, 1, 0);
        winMelody();
        //setRGB(0, 1, 0);
        delay(1500);
        resetGame();
        showMenu();

      } else if (isBoardFull()) {
        gameOver = true;
        setRGB(0, 0, 1);
        delay(1500);
        resetGame();
        showMenu();
      }
    }

  } else if (!gameOver && !turnX) {
    if (gameMode == MODE_AI) moveAI();
    else moveRandom();
    drawBoard();
    if (checkWin('O')) {
      scoreO++;
      gameOver = true;
      setRGB(1, 0, 0);
      lossMelody();
      //setRGB(1, 0, 0);
      delay(1500);
      resetGame();
      showMenu();
    } else if (isBoardFull()) {
      gameOver = true;
      setRGB(0, 0, 1);
      delay(1500);
      resetGame();
      showMenu();
    }
    turnX = true;
  }

  drawBoard();
  delay(50);
}
