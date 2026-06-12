/*
Dual Motor Auto Motion Demo
Stepdance + VelocityGenerator
*/

#define module_driver

#include "stepdance.hpp"

// --------------------------------------------------
// OUTPUT PORTS
// --------------------------------------------------

OutputPort output_a;
OutputPort output_b;

// --------------------------------------------------
// MOTION CHANNELS
// --------------------------------------------------

Channel channel_a;
Channel channel_b;

// --------------------------------------------------
// VELOCITY GENERATORS
// --------------------------------------------------

VelocityGenerator velocityGenA;
VelocityGenerator velocityGenB;

// --------------------------------------------------
// RPC INTERFACE
// --------------------------------------------------

RPC rpc;

// --------------------------------------------------
// AUTO MOTION SETTINGS
// --------------------------------------------------

float min_pos = 0.0;
float max_pos = 5.0;

// motor speeds
float speed_a = 1;
float speed_b = 1;

// estimated positions
float estimated_pos_a = 0.0;
float estimated_pos_b = 0.0;

// directions
int direction_a = 1;
int direction_b = -1;

// timing
uint32_t last_time = 0;
uint32_t last_print = 0;

LoopDelay overhead_delay;

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(115200);

  // --------------------------------------------------
  // OUTPUT A
  // --------------------------------------------------

  output_a.begin(OUTPUT_A);

  channel_a.begin(&output_a, SIGNAL_E);

  channel_a.set_ratio(2, 3200);

  channel_a.invert_output();

  // --------------------------------------------------
  // OUTPUT B
  // --------------------------------------------------

  output_b.begin(OUTPUT_B);

  channel_b.begin(&output_b, SIGNAL_E);

  channel_b.set_ratio(2, 3200);

  channel_b.invert_output();

  // --------------------------------------------------
  // ENABLE DRIVERS
  // --------------------------------------------------

  enable_drivers();

  // --------------------------------------------------
  // GENERATORS
  // --------------------------------------------------

  velocityGenA.begin();
  velocityGenB.begin();

  velocityGenA.output.map(&channel_a.input_target_position);
  velocityGenB.output.map(&channel_b.input_target_position);

  velocityGenA.speed_units_per_sec = speed_a;
  velocityGenB.speed_units_per_sec = speed_b;

  // --------------------------------------------------
  // RPC
  // --------------------------------------------------

  rpc.begin();

  rpc.enroll("set_speed_a", set_speed_a);
  rpc.enroll("set_speed_b", set_speed_b);

  last_time = millis();

  // --------------------------------------------------
  // START STEPDANCE
  // --------------------------------------------------

  dance_start();
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {

  overhead_delay.periodic_call(&report_overhead, 500);

  auto_bounce_motion();

  dance_loop();
}

// --------------------------------------------------
// AUTO MOTION
// --------------------------------------------------

void auto_bounce_motion() {

  uint32_t now = millis();

  float dt = (now - last_time) / 1000.0;

  last_time = now;

  // --------------------------------------------------
  // MOTOR A
  // --------------------------------------------------

  estimated_pos_a += direction_a * speed_a * dt;

  if (estimated_pos_a >= max_pos) {

    estimated_pos_a = max_pos;

    direction_a = -1;
  }

  if (estimated_pos_a <= min_pos) {

    estimated_pos_a = min_pos;

    direction_a = 1;
  }

  velocityGenA.speed_units_per_sec = direction_a * speed_a;

  // --------------------------------------------------
  // MOTOR B
  // --------------------------------------------------

  estimated_pos_b += direction_b * speed_b * dt;

  if (estimated_pos_b >= max_pos) {

    estimated_pos_b = max_pos;

    direction_b = -1;
  }

  if (estimated_pos_b <= min_pos) {

    estimated_pos_b = min_pos;

    direction_b = 1;
  }

  velocityGenB.speed_units_per_sec = direction_b * speed_b;

  // --------------------------------------------------
  // SERIAL PRINT
  // --------------------------------------------------

  if (millis() - last_print > 100) {

    Serial.print("A: ");
    Serial.print(estimated_pos_a, 2);

    Serial.print(" | B: ");
    Serial.print(estimated_pos_b, 2);

    Serial.print(" | SpeedA: ");
    Serial.print(velocityGenA.speed_units_per_sec, 2);

    Serial.print(" | SpeedB: ");
    Serial.println(velocityGenB.speed_units_per_sec, 2);

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