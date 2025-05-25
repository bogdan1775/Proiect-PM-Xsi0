#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// LCD Nokia 5110
#define PIN_SCLK 7
#define PIN_DIN  6
#define PIN_DC   5
#define PIN_CS   4
#define PIN_RST  3
Adafruit_PCD8544 display(PIN_SCLK, PIN_DIN, PIN_DC, PIN_CS, PIN_RST);

// Joystick
#define JOY_X_CHANNEL 0
#define JOY_Y_CHANNEL 1
#define JOY_BTN_BIT 0  // PB0 = D8

// Buzzer
#define BUZZER_BIT 2  // PD2
inline void buzzerOn()  { PORTD |=  _BV(BUZZER_BIT); }
inline void buzzerOff() { PORTD &= ~_BV(BUZZER_BIT); }

#define DEAD_ZONE 100

// LED RGB
#define LED_R 11
#define LED_G 10 
#define LED_B 9

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (r == 0 && g == 0 && b == 0) {

    TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1)); 
    TCCR2A &= ~(1 << COM2A1);                  

    PORTB &= ~((1 << PB1) | (1 << PB2)); 
    PORTB &= ~(1 << PB3);                
  } else {
    
    TCCR1A |= (1 << COM1A1) | (1 << COM1B1);
    TCCR2A |= (1 << COM2A1);

    OCR2A = r;
    OCR1B = g;
    OCR1A = b; 
  }}


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
  ADMUX |= (1 << REFS0);
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

bool readJoystickButton() {
  return !(PINB & (1 << JOY_BTN_BIT));
}

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
    if (selection == 0) display.print("> ");
    display.println("1. Contra AI");
    display.setCursor(0, 20);
    if (selection == 1) display.print("> ");
    display.println("2. Random O");
    display.display();
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Start joc X si 0");
  display.display();
  delay(1000);
}

void playTone(uint16_t freqHz, uint16_t durMs) {
  const uint32_t halfPeriod_us = 500000UL / freqHz;
  uint32_t cycles = (uint32_t)freqHz * durMs / 1000UL;
  for (uint32_t i = 0; i < cycles; ++i) {
    buzzerOn();
    delayMicroseconds(halfPeriod_us);
    buzzerOff();
    delayMicroseconds(halfPeriod_us);
  }
}

void winMelody() {
  playTone(262, 200); delay(50);
  playTone(330, 200); delay(50);
  playTone(392, 200); delay(50);
  playTone(523, 300); delay(50);
}
void lossMelody() {
  playTone(392, 200); delay(50);
  playTone(330, 200); delay(50);
  playTone(262, 300); delay(50);
}

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
    if (selection == 0) display.print("> ");
    display.println("Da");
    display.setCursor(0, 27);
    if (selection == 1) display.print("> ");
    display.println("Nu");
    display.display();
  }
}

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
    if (board[i][0] == player && board[i][1] == player && board[i][2] == player) return true;
    if (board[0][i] == player && board[1][i] == player && board[2][i] == player) return true;
  }
  return (board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
         (board[0][2] == player && board[1][1] == player && board[2][0] == player);
}

bool isBoardFull() {
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 3; x++)
      if (board[y][x] == ' ') return false;
  return true;
}

int minimax(bool isMaximizing, int depth) {
  if (checkWin('O')) return 17;
  if (checkWin('X')) return -17;
  if (isBoardFull()) return 0;

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
    setRGB(255, 0, 0);
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
    setRGB(255, 0, 0);
  }
  delay(1000);
}

void setup() {
  Serial.begin(9600);

  // Buzzer
  DDRD |= _BV(BUZZER_BIT);

  // Joystick button
  DDRB &= ~(1 << JOY_BTN_BIT);
  PORTB |= (1 << JOY_BTN_BIT);

  // ADC
  ADCSRA |= (1 << ADEN);


  DDRB |= (1 << DDB3);
  DDRB |= (1 << DDB2);
  DDRB |= (1 << DDB1);

  TCCR2A = (1 << COM2A1) | (1 << WGM20);  
  TCCR2B = (1 << CS21); 

  
  TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10); 
  TCCR1B = (1 << WGM12) | (1 << CS11);
  
  TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1));
  TCCR2A &= ~(1 << COM2A1);                   

  PORTB &= ~((1 << PB1) | (1 << PB2));
  PORTB &= ~(1 << PB3); 

  display.begin();
  display.setContrast(60);
  showMenu();
  resetGame();
}

void loop() {
  int x = readAnalog(JOY_X_CHANNEL);
  int y = readAnalog(JOY_Y_CHANNEL);
  bool btn = readJoystickButton();

  if (!gameOver && turnX) {
    setRGB(0, 0, 255);

    if (x < 512 - DEAD_ZONE && cursorX > 0) cursorX--;
    if (x > 512 + DEAD_ZONE && cursorX < 2) cursorX++;
    if (y < 512 - DEAD_ZONE && cursorY > 0) cursorY--;
    if (y > 512 + DEAD_ZONE && cursorY < 2) cursorY++;
    delay(150);

    if (btn && board[cursorY][cursorX] == ' ') {
      board[cursorY][cursorX] = 'X';
      turnX = false;
      if (checkWin('X')) {
        scoreX++;
        gameOver = true;
        winMelody();
        setRGB(0, 255, 0);
        delay(1500);
        resetGame();
        showMenu();
      } else if (isBoardFull()) {
        gameOver = true;
        setRGB(0, 0, 255);
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
      lossMelody();
      setRGB(255, 0, 0);
      delay(1500);
      resetGame();
      showMenu();
    } else if (isBoardFull()) {
      gameOver = true;
      setRGB(0, 0, 255);
      delay(1500);
      resetGame();
      showMenu();
    }
    turnX = true;
  }

  drawBoard();
  delay(50);
}
