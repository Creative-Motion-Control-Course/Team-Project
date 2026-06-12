/*
Triple Motor + Continuous Rotation Servo + D1 Toggle Button

OUTPUT_A = X axis
OUTPUT_B = Z axis
OUTPUT_C = Spin stepper motor
OUTPUT_D = Continuous rotation servo

D1 button = ON/OFF toggle
*/

#define module_driver

#include "stepdance.hpp"

// --------------------------------------------------
// OUTPUT PORTS
// --------------------------------------------------

OutputPort output_a;
OutputPort output_b;
OutputPort output_c;
OutputPort output_d;

// --------------------------------------------------
// MOTION CHANNELS
// --------------------------------------------------

Channel channel_a;
Channel channel_b;
Channel channel_c;
Channel channel_servo;

// --------------------------------------------------
// GENERATORS
// --------------------------------------------------

VelocityGenerator velocityGenA;
VelocityGenerator velocityGenB;
VelocityGenerator velocityGenC;

PositionGenerator servoGen;

// --------------------------------------------------
// BUTTON
// --------------------------------------------------

Button button_d1;

bool system_on = false;

// --------------------------------------------------
// RPC
// --------------------------------------------------

RPC rpc;

// --------------------------------------------------
// MOTION SETTINGS
// --------------------------------------------------

float min_pos = 0.0;

float max_pos_a = 100.0;
float max_pos_b = 100.0;

float speed_a = 100.0;
float speed_b = 100.0;

float spin_speed_c = 500.0;

// servo values
float servo_stop_value = 90.0;
float servo_run_value  = 180.0;
float servo_move_speed = 2000.0;

// estimated positions
float estimated_pos_a = 0.0;
float estimated_pos_b = 0.0;

int direction_a = 1;
int direction_b = 1;

uint32_t last_time = 0;
uint32_t last_print = 0;

LoopDelay overhead_delay;

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(115200);

  // --------------------------------------------------
  // OUTPUT A = X
  // --------------------------------------------------

  output_a.begin(OUTPUT_A);
  channel_a.begin(&output_a, SIGNAL_E);
  channel_a.set_ratio(40, 3200);
  channel_a.invert_output();

  // --------------------------------------------------
  // OUTPUT B = Z
  // --------------------------------------------------

  output_b.begin(OUTPUT_B);
  channel_b.begin(&output_b, SIGNAL_E);
  channel_b.set_ratio(40, 3200);

  // --------------------------------------------------
  // OUTPUT C = SPIN STEPPER
  // --------------------------------------------------

  output_c.begin(OUTPUT_C);
  channel_c.begin(&output_c, SIGNAL_E);
  channel_c.set_ratio(360, 3200);

  // --------------------------------------------------
  // OUTPUT D = SERVO
  // --------------------------------------------------

  output_d.begin(OUTPUT_D);
  channel_servo.begin(&output_d, SIGNAL_E);
  channel_servo.set_ratio(1, 1);

  // --------------------------------------------------
  // ENABLE DRIVERS
  // --------------------------------------------------

  enable_drivers();

  // --------------------------------------------------
  // GENERATORS
  // --------------------------------------------------

  velocityGenA.begin();
  velocityGenB.begin();
  velocityGenC.begin();
  servoGen.begin();

  velocityGenA.output.map(&channel_a.input_target_position);
  velocityGenB.output.map(&channel_b.input_target_position);
  velocityGenC.output.map(&channel_c.input_target_position);
  servoGen.output.map(&channel_servo.input_target_position);

  // start stopped
  velocityGenA.speed_units_per_sec = 0;
  velocityGenB.speed_units_per_sec = 0;
  velocityGenC.speed_units_per_sec = 0;

  servoGen.go(servo_stop_value, ABSOLUTE, servo_move_speed);

  // --------------------------------------------------
  // BUTTON D1
  // --------------------------------------------------

  button_d1.begin(IO_D1, INPUT_PULLDOWN);
  button_d1.set_mode(BUTTON_MODE_TOGGLE);
  button_d1.set_callback_on_press(&toggle_system);

  // --------------------------------------------------
  // RPC
  // --------------------------------------------------

  rpc.begin();

  rpc.enroll("set_speed_a", set_speed_a);
  rpc.enroll("set_speed_b", set_speed_b);
  rpc.enroll("set_spin_speed_c", set_spin_speed_c);

  last_time = millis();

  // --------------------------------------------------
  // START
  // --------------------------------------------------

  dance_start();

  stop_all_motion();
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {

  overhead_delay.periodic_call(&report_overhead, 500);

  if (system_on) {
    auto_bounce_motion();
  } else {
    stop_all_motion();
  }

  dance_loop();
}

// --------------------------------------------------
// BUTTON FUNCTION
// --------------------------------------------------

void toggle_system() {

  system_on = !system_on;

  if (system_on) {
    start_all_motion();
    Serial.println("SYSTEM: ON");
  } else {
    stop_all_motion();
    Serial.println("SYSTEM: OFF");
  }
}

// --------------------------------------------------
// START / STOP
// --------------------------------------------------

void start_all_motion() {

  last_time = millis();

  velocityGenC.speed_units_per_sec = spin_speed_c;

  servoGen.go(servo_run_value, ABSOLUTE, servo_move_speed);
}

void stop_all_motion() {

  velocityGenA.speed_units_per_sec = 0;
  velocityGenB.speed_units_per_sec = 0;
  velocityGenC.speed_units_per_sec = 0;

  servoGen.go(servo_stop_value, ABSOLUTE, servo_move_speed);
}

// --------------------------------------------------
// AUTO BOUNCE MOTION
// --------------------------------------------------

void auto_bounce_motion() {

  uint32_t now = millis();

  float dt = (now - last_time) / 1000.0;

  last_time = now;

  // X AXIS
  estimated_pos_a += direction_a * speed_a * dt;

  if (estimated_pos_a >= max_pos_a) {
    estimated_pos_a = max_pos_a;
    direction_a = -1;
  }

  if (estimated_pos_a <= min_pos) {
    estimated_pos_a = min_pos;
    direction_a = 1;
  }

  velocityGenA.speed_units_per_sec = direction_a * speed_a;

  // Z AXIS
  estimated_pos_b += direction_b * speed_b * dt;

  if (estimated_pos_b >= max_pos_b) {
    estimated_pos_b = max_pos_b;
    direction_b = -1;
  }

  if (estimated_pos_b <= min_pos) {
    estimated_pos_b = min_pos;
    direction_b = 1;
  }

  velocityGenB.speed_units_per_sec = direction_b * speed_b;

  // SERIAL PRINT
  if (millis() - last_print > 100) {

    Serial.print("System: ");
    Serial.print(system_on ? "ON" : "OFF");

    Serial.print(" | X: ");
    Serial.print(estimated_pos_a, 2);

    Serial.print(" | Z: ");
    Serial.print(estimated_pos_b, 2);

    Serial.print(" | Spin: ");
    Serial.print(spin_speed_c, 2);

    Serial.print(" | Servo: ");
    Serial.println(system_on ? "RUN" : "STOP");

    last_print = millis();
  }
}

// --------------------------------------------------
// CPU USAGE
// --------------------------------------------------

void report_overhead() {

  Serial.print("CPU Usage: ");

  Serial.println(stepdance_get_cpu_usage(), 4);
}

// --------------------------------------------------
// RPC FUNCTIONS
// --------------------------------------------------

void set_speed_a(float32_t new_speed) {

  speed_a = abs(new_speed);
}

void set_speed_b(float32_t new_speed) {

  speed_b = abs(new_speed);
}

void set_spin_speed_c(float32_t new_speed) {

  spin_speed_c = new_speed;

  if (system_on) {
    velocityGenC.speed_units_per_sec = spin_speed_c;
  }
}