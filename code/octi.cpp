/************************************************
 * Roopantrics – Octi Animated Creature
 * Board: NodeMCU v2 (ESP8266)
 ************************************************/

#include <Arduino.h>

/* ===================== PIN MAPPING ===================== */
// Tilt motor (Base) - T
#define T1 D1
#define T2 D2
#define T3 D5
#define T4 D6

// Rotate motor (Top) - R
#define R1 D7
#define R2 D8
#define R3 D3
#define R4 D4


/* ===================== STEPPER SEQUENCE ===================== */
const int STEP_SEQ[8][4] = {
  {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
  {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};

/* ===================== MOTOR STRUCT ===================== */
struct Motor {
  int pins[4];
  int stepIndex;
  int direction;
  int stepDelay;
};

/* ===================== MULTIPLIERS ===================== */
// Tilt motor: 3× speed
const float TILT_SPEED_MULT = 0.33;

// Tilt motor: 1.5× duration
const float TILT_TIME_MULT  = 1.5;

// Shake amplitude: (+2, 0, −2)
const int SHAKE_AMPLITUDE_MULT = 2;

/* ===================== MOTORS ===================== */
Motor tiltMotor   = {{T1,T2,T3,T4}, 0, 1, int(9 * TILT_SPEED_MULT)};
Motor rotateMotor = {{R1,R2,R3,R4}, 4, 1, 6};

/* ===================== PARAMETERS ===================== */
// Short, bursty tilt actions
const unsigned long TILT_BASE_MIN_MS = 350;
const unsigned long TILT_BASE_MAX_MS = 700;

// Rotate limits (~1/6 rotation)
const int MAX_ROT_STEPS = 340;

// Shake parameters (base units)
const int SHAKE_STEPS_MIN  = 8;
const int SHAKE_STEPS_MAX  = 16;
const int SHAKE_CYCLES_MIN = 6;
const int SHAKE_CYCLES_MAX = 18;
const int SHAKE_DELAY = 10;

unsigned long lastShakeTime = 0;
const unsigned long SHAKE_COOLDOWN_MS = 4500;

/* ===================== LOW LEVEL STEP ===================== */
void stepMotor(Motor &m) {
  for (int i = 0; i < 4; i++)
    digitalWrite(m.pins[i], STEP_SEQ[m.stepIndex][i]);

  m.stepIndex = (m.stepIndex + m.direction + 8) % 8;
  delay(m.stepDelay);
}

/* ===================== EASED MOVE ===================== */
void easedMove(Motor &m, int steps, int slowDelay, int fastDelay) {
  for (int i = 0; i < steps; i++) {
    if (i < steps * 0.25 || i > steps * 0.75)
      m.stepDelay = slowDelay;
    else
      m.stepDelay = fastDelay;

    stepMotor(m);

    if (random(0,100) < 5)
      delay(random(4, 15));
  }
}

/* ===================== SHAKE EVENT ===================== */
void shakeMotor(Motor &m) {

  int cycles = random(SHAKE_CYCLES_MIN, SHAKE_CYCLES_MAX);

  // ↑ amplitude multiplied here
  int steps = random(SHAKE_STEPS_MIN, SHAKE_STEPS_MAX)
              * SHAKE_AMPLITUDE_MULT;

  m.stepDelay = SHAKE_DELAY;

  for (int c = 0; c < cycles; c++) {
    m.direction = 1;
    for (int i = 0; i < steps; i++) stepMotor(m);

    m.direction = -1;
    for (int i = 0; i < steps; i++) stepMotor(m);
  }

  lastShakeTime = millis();
}

/* ===================== ROTATE ACTION ===================== */
void rotateAction() {
  int usedSteps = 0;
  int packets = random(2, 4);

  for (int p = 0; p < packets && usedSteps < MAX_ROT_STEPS; p++) {
    rotateMotor.direction = random(0,2) ? 1 : -1;

    int remaining = MAX_ROT_STEPS - usedSteps;
    int steps = random(60, 120);
    if (steps > remaining) steps = remaining;

    easedMove(rotateMotor, steps, 8, 5);
    usedSteps += steps;
  }
}

/* ===================== TILT ACTION ===================== */
void tiltAction() {

  unsigned long duration = random(
    TILT_BASE_MIN_MS * TILT_TIME_MULT,
    TILT_BASE_MAX_MS * TILT_TIME_MULT
  );

  unsigned long start = millis();
  tiltMotor.direction = random(0,2) ? 1 : -1;

  while (millis() - start < duration) {

    easedMove(
      tiltMotor,
      random(25, 50),
      int(13 * TILT_SPEED_MULT),
      int(9  * TILT_SPEED_MULT)
    );

    // balance wobble
    if (random(0,100) < 18) {
      tiltMotor.direction *= -1;
      easedMove(
        tiltMotor,
        random(8, 14),
        int(15 * TILT_SPEED_MULT),
        int(11 * TILT_SPEED_MULT)
      );
      tiltMotor.direction *= -1;
    }

    // head reacts DURING tilt
    if (random(0,100) < 18) {
      easedMove(rotateMotor, random(8, 20), 11, 7);
    }
  }
}

/* ===================== MICRO MOTION ===================== */
void microMotion() {
  if (random(0,100) < 60)
    easedMove(rotateMotor, random(8, 16), 11, 7);
  else
    easedMove(
      tiltMotor,
      random(8, 18),
      int(14 * TILT_SPEED_MULT),
      int(10 * TILT_SPEED_MULT)
    );
}

/* ===================== SETUP ===================== */
void setup() {
  int pins[] = {T1,T2,T3,T4,R1,R2,R3,R4};
  for (int i = 0; i < 8; i++) pinMode(pins[i], OUTPUT);

  Serial.begin(115200);
  delay(500);
  randomSeed(analogRead(A0));

  Serial.println("Roopantrics – Organic Creature Motion (FAST, SHAKE ×2)");
}

/* ===================== LOOP ===================== */
void loop() {

  int r = random(0, 100);

  // Rare shake (now higher amplitude)
  if (millis() - lastShakeTime > SHAKE_COOLDOWN_MS && r < 6) {
    shakeMotor(random(0,100) < 65 ? rotateMotor : tiltMotor);
    microMotion();
    return;
  }

  // Primary action
  if (r < 45)       rotateAction();
  else if (r < 90)  tiltAction();
  else              microMotion();

  // Action chaining
  if (random(0,100) < 55) {
    //if (random(0,100) < 50)
    if (random(0,100) < 35)
      rotateAction();
    else
      tiltAction();
  }

  microMotion();   // never dead
}
