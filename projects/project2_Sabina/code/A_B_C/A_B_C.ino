/*
Triple Motor Auto Motion Demo

OUTPUT_A = X axis
OUTPUT_B = Z axis
OUTPUT_C = Spin motor

A/B = bouncing motion
C = continuous spin
*/

#define module_driver

#include "stepdance.hpp"

// --------------------------------------------------
// OUTPUT PORTS
// --------------------------------------------------

OutputPort output_a;
OutputPort output_b;
OutputPort output_c;

// --------------------------------------------------
// MOTION CHANNELS
// --------------------------------------------------

Channel channel_a;
Channel channel_b;
Channel channel_c;

// --------------------------------------------------
// VELOCITY GENERATORS
// --------------------------------------------------

VelocityGenerator velocityGenA;
VelocityGenerator velocityGenB;
VelocityGenerator velocityGenC;

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

// spin speed
float spin_speed_c = 500.0;

// estimated positions
float estimated_pos_a = 0.0;
float estimated_pos_b = 0.0;

// directions
int direction_a = 1;
int direction_b = 1;

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

  // channel_b.invert_output();

  // --------------------------------------------------
  // OUTPUT C = SPIN
  // --------------------------------------------------

  output_c.begin(OUTPUT_C);

  channel_c.begin(&output_c, SIGNAL_E);

  // rotational axis
  channel_c.set_ratio(360, 3200);

  // invert if needed
  // channel_c.invert_output();

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

  velocityGenA.output.map(&channel_a.input_target_position);

  velocityGenB.output.map(&channel_b.input_target_position);

  velocityGenC.output.map(&channel_c.input_target_position);

  velocityGenA.speed_units_per_sec = speed_a;

  velocityGenB.speed_units_per_sec = speed_b;

  // continuous spin
  velocityGenC.speed_units_per_sec = spin_speed_c;

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
// AUTO BOUNCE MOTION
// --------------------------------------------------

void auto_bounce_motion() {

  uint32_t now = millis();

  float dt = (now - last_time) / 1000.0;

  last_time = now;

  // --------------------------------------------------
  // X AXIS
  // --------------------------------------------------

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

  // --------------------------------------------------
  // Z AXIS
  // --------------------------------------------------

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

  // --------------------------------------------------
  // SERIAL PRINT
  // --------------------------------------------------

  if (millis() - last_print > 100) {

    Serial.print("X: ");
    Serial.print(estimated_pos_a, 2);

    Serial.print(" | Z: ");
    Serial.print(estimated_pos_b, 2);

    Serial.print(" | Spin: ");
    Serial.println(spin_speed_c, 2);

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

  velocityGenC.speed_units_per_sec = spin_speed_c;
}