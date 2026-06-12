/*
Auto Up-Down Single Motor Demo
Stepdance + VelocityGenerator
*/

#define module_driver

#include "stepdance.hpp"

// --------------------------------------------------
// OUTPUT PORT
// --------------------------------------------------

OutputPort output_a;

// --------------------------------------------------
// MOTION CHANNEL
// --------------------------------------------------

Channel channel_a;

// --------------------------------------------------
// VELOCITY GENERATOR
// --------------------------------------------------

VelocityGenerator velocityGen;

// --------------------------------------------------
// RPC INTERFACE
// --------------------------------------------------

RPC rpc;

// --------------------------------------------------
// AUTO MOTION SETTINGS
// --------------------------------------------------

// Safe slow settings for first test

float min_pos = 0.0;
float max_pos = 5.0;

float speed = 0.8;

// estimated position
float estimated_pos = 0.0;

// direction
int direction = 1;

// timing
uint32_t last_time = 0;
uint32_t last_print = 0;

LoopDelay overhead_delay;

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(115200);

  // Start output port
  output_a.begin(OUTPUT_A);

  // Enable drivers
  enable_drivers();

  // Start motion channel
  channel_a.begin(&output_a, SIGNAL_E);

  // 2mm lead screw
  // 3200 microsteps per revolution
  channel_a.set_ratio(2, 3200);

  // Uncomment if direction is reversed
  channel_a.invert_output();

  // Start velocity generator
  velocityGen.begin();

  // Connect generator to motion channel
  velocityGen.output.map(&channel_a.input_target_position);

  // Initial speed
  velocityGen.speed_units_per_sec = speed;

  // RPC
  rpc.begin();

  rpc.enroll("set_speed", set_speed);
  rpc.enroll("set_range", set_range);

  last_time = millis();

  // Start Stepdance
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

  // Estimate position
  estimated_pos += direction * speed * dt;

  // Upper limit
  if (estimated_pos >= max_pos) {

    estimated_pos = max_pos;

    direction = -1;
  }

  // Lower limit
  if (estimated_pos <= min_pos) {

    estimated_pos = min_pos;

    direction = 1;
  }

  // Update velocity direction
  velocityGen.speed_units_per_sec = direction * speed;

  // Print position every 100ms
  if (millis() - last_print > 100) {

    Serial.print("Position: ");

    Serial.print(estimated_pos, 2);

    Serial.print(" | Speed: ");

    Serial.println(velocityGen.speed_units_per_sec, 2);

    last_print = millis();
  }
}

// --------------------------------------------------
// CPU USAGE REPORT
// --------------------------------------------------

void report_overhead() {

  Serial.print("CPU Usage: ");

  Serial.println(stepdance_get_cpu_usage(), 4);
}

// --------------------------------------------------
// RPC FUNCTIONS
// --------------------------------------------------

void set_speed(float32_t new_speed) {

  speed = abs(new_speed);
}

void set_range(float32_t new_min, float32_t new_max) {

  min_pos = new_min;

  max_pos = new_max;
}