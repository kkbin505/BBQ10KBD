#include <Arduino.h>
#include <ctype.h>
#include <BleKeyboard.h>

BleKeyboard Keyboard("BBQ10-BLE2", "BBQ10", 100);

constexpr uint8_t rows[] = {6, 7, 8, 9, 10, 11, 12};
constexpr uint8_t cols[] = {1, 2, 3, 4, 5};

constexpr size_t ROW_COUNT = sizeof(rows) / sizeof(rows[0]);
constexpr size_t COL_COUNT = sizeof(cols) / sizeof(cols[0]);

bool keys[COL_COUNT][ROW_COUNT] = {};
bool lastValue[COL_COUNT][ROW_COUNT] = {};
bool changedValue[COL_COUNT][ROW_COUNT] = {};

constexpr unsigned long LONG_PRESS_MS = 280;

constexpr size_t D_COL = 1;
constexpr size_t D_ROW = 2;
constexpr size_t H_COL = 3;
constexpr size_t H_ROW = 1;
constexpr size_t L_COL = 4;
constexpr size_t L_ROW = 1;
constexpr size_t BACKSPACE_COL = 4;
constexpr size_t BACKSPACE_ROW = 3;
// [col][row]
const char keyboard[COL_COUNT][ROW_COUNT] = {
  {'q', 'w', '\0', 'a', '\0', ' ', '\0'},
  {'e', 's', 'd', 'p', 'x', 'z', '\0'},
  {'r', 'g', 't', '\0', 'v', 'c', 'f'},
  {'u', 'h', 'y', '\0', 'b', 'n', 'j'},
  {'o', 'l', 'i', '\0', '$', 'm', 'k'},
};

const char keyboardSymbol[COL_COUNT][ROW_COUNT] = {
  {'#', '1', '\0', '*', '\0', '\0', '0'},
  {'2', '4', '5', '@', '8', '7', '\0'},
  {'3', '/', '(', '\0', '?', '9', '6'},
  {'_', ':', ')', '\0', '!', ',', ';'},
  {'+', '"', '-', '\0', '\0', '.', '\''},
};

bool symbolSelected = false;

struct LongPressKey {
  size_t col;
  size_t row;
  char shortOutput;
  char longOutput;
  bool tracking;
  bool longSent;
  unsigned long pressStart;
};

LongPressKey longPressKeys[] = {
  {D_COL, D_ROW, 'd', 'f', false, false, 0},
  {H_COL, H_ROW, 'h', 'j', false, false, 0},
  {L_COL, L_ROW, 'l', 'k', false, false, 0},
};

bool keyPressed(size_t colIndex, size_t rowIndex) {
  return changedValue[colIndex][rowIndex] && keys[colIndex][rowIndex];
}

bool keyActive(size_t colIndex, size_t rowIndex) {
  return keys[colIndex][rowIndex];
}

bool isPrintableKey(size_t colIndex, size_t rowIndex) {
  return keyboard[colIndex][rowIndex] != '\0' || keyboardSymbol[colIndex][rowIndex] != '\0';
}

bool isLongPressManagedKey(size_t colIndex, size_t rowIndex) {
  for (size_t i = 0; i < (sizeof(longPressKeys) / sizeof(longPressKeys[0])); i++) {
    if (longPressKeys[i].col == colIndex && longPressKeys[i].row == rowIndex) {
      return true;
    }
  }
  return false;
}

char applyShiftIfNeeded(char output) {
  // Shift keys: [1][6] (left), [2][3] (right)
  if (keyActive(1, 6) || keyActive(2, 3)) {
    return static_cast<char>(toupper(static_cast<unsigned char>(output)));
  }
  return output;
}

void emitKey(char output) {
  if (output == '\0') {
    return;
  }
  Keyboard.write(static_cast<uint8_t>(applyShiftIfNeeded(output)));
}

void processLongPressFallbacks() {
  const unsigned long now = millis();

  for (size_t i = 0; i < (sizeof(longPressKeys) / sizeof(longPressKeys[0])); i++) {
    LongPressKey &key = longPressKeys[i];
    const bool active = keyActive(key.col, key.row);

    if (active) {
      if (!key.tracking) {
        key.tracking = true;
        key.longSent = false;
        key.pressStart = now;
      } else if (!key.longSent && (now - key.pressStart) >= LONG_PRESS_MS) {
        emitKey(key.longOutput);
        key.longSent = true;
      }
    } else if (key.tracking) {
      if (!key.longSent) {
        emitKey(key.shortOutput);
      }
      key.tracking = false;
      key.longSent = false;
    }
  }
}

void readMatrix() {
  for (size_t colIndex = 0; colIndex < COL_COUNT; colIndex++) {
    const uint8_t curCol = cols[colIndex];

    pinMode(curCol, OUTPUT);
    digitalWrite(curCol, LOW);

    for (size_t rowIndex = 0; rowIndex < ROW_COUNT; rowIndex++) {
      const uint8_t curRow = rows[rowIndex];
      pinMode(curRow, INPUT_PULLUP);
      delayMicroseconds(200);

      const bool buttonPressed = (digitalRead(curRow) == LOW);
      keys[colIndex][rowIndex] = buttonPressed;
      changedValue[colIndex][rowIndex] = (lastValue[colIndex][rowIndex] != buttonPressed);
      lastValue[colIndex][rowIndex] = buttonPressed;

      pinMode(curRow, INPUT);
    }

    pinMode(curCol, INPUT);
  }

  // Symbol key at col1/row3 => index [0][2]
  if (keyPressed(0, 2)) {
    symbolSelected = true;
  }
}

void printMatrix() {
  for (size_t rowIndex = 0; rowIndex < ROW_COUNT; rowIndex++) {
    for (size_t colIndex = 0; colIndex < COL_COUNT; colIndex++) {
      if (!keyPressed(colIndex, rowIndex)) {
        continue;
      }

      if (colIndex == BACKSPACE_COL && rowIndex == BACKSPACE_ROW) {
        Keyboard.write(KEY_BACKSPACE);
        continue;
      }

      if (!isPrintableKey(colIndex, rowIndex)) {
        continue;
      }

      if (!symbolSelected && isLongPressManagedKey(colIndex, rowIndex)) {
        continue;
      }

      char output = keyboard[colIndex][rowIndex];
      if (symbolSelected) {
        symbolSelected = false;
        if (keyboardSymbol[colIndex][rowIndex] != '\0') {
          output = keyboardSymbol[colIndex][rowIndex];
        }
      }

      if (output == '\0') {
        continue;
      }

      emitKey(output);
    }
  }
}

void setup() {
  Keyboard.setDelay(8);
  Keyboard.begin();
  delay(200);

  for (size_t i = 0; i < ROW_COUNT; i++) {
    pinMode(rows[i], INPUT);
  }

  for (size_t i = 0; i < COL_COUNT; i++) {
    pinMode(cols[i], INPUT_PULLUP);
  }
}

void loop() {
  readMatrix();
  processLongPressFallbacks();
  printMatrix();

  // Enter key at [3][3]
  if (keyPressed(3, 3)) {
    Keyboard.write(KEY_RETURN);
  }

  delay(10);
}
