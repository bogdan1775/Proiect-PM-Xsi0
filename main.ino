#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// LCD Nokia 5110
#define PIN_SCLK 7
#define PIN_DIN  6
#define PIN_DC   5
#define PIN_CS   4
#define PIN_RST  3
Adafruit_PCD8544 display = Adafruit_PCD8544(PIN_SCLK, PIN_DIN, PIN_DC, PIN_CS, PIN_RST);

// Joystick
#define JOY_X A0
#define JOY_Y A1
#define JOY_BTN 8

// Buzzer
#define BUZZER_PIN 2

// LED RGB (catod comun)
#define LED_R 11
#define LED_G 10
#define LED_B 9

#define DEAD_ZONE 100

char board[3][3];
int cursorX = 0;
int cursorY = 0;
bool turnX = true;
bool gameOver = false;

int scoreX = 0;
int scoreO = 0;

enum GameMode { MODE_AI, MODE_RANDOM };
GameMode gameMode = MODE_AI;

void showMenu() {
  int selection = 0;
  bool selected = false;
  unsigned long lastMove = 0;

  while (!selected) {
    int y = analogRead(JOY_Y);
    if (millis() - lastMove > 200) {
      if (y < 512 - DEAD_ZONE && selection > 0) {
        selection = 0;
        lastMove = millis();
      } else if (y > 512 + DEAD_ZONE && selection < 1) {
        selection = 1;
        lastMove = millis();
      }
    }
    if (digitalRead(JOY_BTN) == LOW) {
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

void winMelody() {
  tone(BUZZER_PIN, 262, 200); // Do
  delay(250);
  tone(BUZZER_PIN, 330, 200); // Mi
  delay(250);
  tone(BUZZER_PIN, 392, 200); // Sol
  delay(250);
  tone(BUZZER_PIN, 523, 300); // Do 
  delay(350);
  noTone(BUZZER_PIN);
}

void lossMelody() {
  tone(BUZZER_PIN, 392, 200); // Sol
  delay(250);
  tone(BUZZER_PIN, 330, 200); // Mi
  delay(250);
  tone(BUZZER_PIN, 262, 300); // Do
  delay(350);
  noTone(BUZZER_PIN);
}

bool resetScore() {
  int selection = 0;
  unsigned long lastMove = 0;

  while (true) {
    int y = analogRead(JOY_Y);
    if (millis() - lastMove > 200) {
      if (y < 512 - DEAD_ZONE && selection > 0) {
        selection = 0;
        lastMove = millis();
      } else if (y > 512 + DEAD_ZONE && selection < 1) {
        selection = 1;
        lastMove = millis();
      }
    }

    if (digitalRead(JOY_BTN) == LOW) {
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

  //showMenu();
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
  if (gameMode == MODE_AI) 
    display.print("AI");
  else 
    display.print("Rnd");

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
  if (board[0][0] == player && board[1][1] == player && board[2][2] == player) return true;
  if (board[0][2] == player && board[1][1] == player && board[2][0] == player) return true;
  return false;
}

bool isBoardFull() {
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 3; x++)
      if (board[y][x] == ' ') return false;
  return true;
}

int minimax(bool isMaximizing) {
  if (checkWin('O')) return 17;
  if (checkWin('X')) return -17;
  if (isBoardFull()) return 0;

  if (isMaximizing) {
    int bestScore = -1000;
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        if (board[y][x] == ' ') {
          board[y][x] = 'O';
          int score = minimax(false);
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
          int score = minimax(true);
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
        int score = minimax(false);
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
    analogWrite(LED_R, 255);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 0);
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
    analogWrite(LED_R, 255);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 0);
  }
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  display.begin();
  display.setContrast(60);

  showMenu();
  resetGame();
}

void loop() {
  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);
  int btn = digitalRead(JOY_BTN);

  if (!gameOver && turnX) {
    analogWrite(LED_R, 0);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 255);

    if (x < 512 - DEAD_ZONE && cursorX > 0) cursorX--;
    if (x > 512 + DEAD_ZONE && cursorX < 2) cursorX++;
    if (y < 512 - DEAD_ZONE && cursorY > 0) cursorY--;
    if (y > 512 + DEAD_ZONE && cursorY < 2) cursorY++;
    delay(150);

    if (btn == LOW && board[cursorY][cursorX] == ' ') {
      board[cursorY][cursorX] = 'X';
      turnX = false;
      //drawBoard();
      if (checkWin('X')) {
        scoreX++;
        gameOver = true;
        // tone(BUZZER_PIN, 1500, 500);
        winMelody();
        analogWrite(LED_R, 0);
        analogWrite(LED_G, 255);
        analogWrite(LED_B, 0);
        delay(1500);
        resetGame();
        showMenu();
      } else if (isBoardFull()) {
        gameOver = true;
        tone(BUZZER_PIN, 1000, 500);
        analogWrite(LED_R, 0);
        analogWrite(LED_G, 0);
        analogWrite(LED_B, 255);
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
      //tone(BUZZER_PIN, 500, 500);
      lossMelody();
      analogWrite(LED_R, 255);
      analogWrite(LED_G, 0);
      analogWrite(LED_B, 0);
      delay(1500);
      resetGame();
      showMenu();
    } else if (isBoardFull()) {
      gameOver = true;
      tone(BUZZER_PIN, 1000, 500);
      analogWrite(LED_R, 0);
      analogWrite(LED_G, 0);
      analogWrite(LED_B, 255);
      delay(1500);
      resetGame();
      showMenu();
    }
    turnX = true;
  }

  drawBoard();
  delay(50);
}
