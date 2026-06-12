/*
Triple Motor Auto Motion Demo

OUTPUT_A = A motor
OUTPUT_B = B motor / X axis exact 0 <-> range bounce
OUTPUT_C = Spin motor

ENCODER_1 = A motor speed
IO_A1     = A motor range

ENCODER_2 = B motor speed
IO_A2     = B motor range

IO_A3     = C motor spin speed

IO_D1     = Push button toggle
            Press once = ON
            Press again = OFF
*/

#define module_driver

#include "stepdance.hpp"

// --------------------------------------------------
// INPUTS
// --------------------------------------------------

Encoder encoder_1;
Encoder encoder_2;

AnalogInput analog_a1;
AnalogInput analog_a2;
AnalogInput analog_a3;

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
// GENERATORS
// --------------------------------------------------

VelocityGenerator velocityGenA;
PositionGenerator positionGenB;
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

float speed_a = 50.0;
float speed_b = 50.0;

float min_range_a = 10.0;
float max_range_a = 300.0;

float min_range_b = 10.0;
float max_range_b = 300.0;

float min_spin_speed_c = 0.0;
float max_spin_speed_c = 300.0;

float min_speed_a = 5.0;
float max_speed_a = 200.0;

float min_speed_b = 5.0;
float max_speed_b = 200.0;

float encoder_speed_step_a = 1.0;
float encoder_speed_step_b = 1.0;

float last_encoder_value_a = 0.0;
float last_encoder_value_b = 0.0;

float spin_speed_c = 0.0;

// estimated positions
float estimated_pos_a = 0.0;
float estimated_pos_b = 0.0;

// directions
int direction_a = 1;

// B position bounce state
bool b_moving_to_max = true;
bool b_command_sent = false;

// system on/off
bool system_on = false;

// button debounce
bool last_button_reading = HIGH;
bool stable_button_state = HIGH;
uint32_t last_debounce_time = 0;
uint32_t debounce_delay = 40;

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
  // INPUT SETUP
  // --------------------------------------------------

  analog_a1.set_floor(min_range_a, 25);
  analog_a1.set_ceiling(max_range_a, 1020);
  analog_a1.begin(IO_A1);

  analog_a2.set_floor(min_range_b, 25);
  analog_a2.set_ceiling(max_range_b, 1020);
  analog_a2.begin(IO_A2);

  analog_a3.set_floor(min_spin_speed_c, 25);
  analog_a3.set_ceiling(max_spin_speed_c, 1020);
  analog_a3.begin(IO_A3);

  encoder_1.begin(ENCODER_1);
  encoder_1.set_ratio(1.0, 1.0);

  encoder_2.begin(ENCODER_2);
  encoder_2.set_ratio(1.0, 1.0);

  last_encoder_value_a = encoder_1.read();
  last_encoder_value_b = encoder_2.read();

  // D1 push button
  // wiring: one side to IO_D1, other side to GND
  pinMode(IO_D1, INPUT_PULLUP);

  // --------------------------------------------------
  // OUTPUT A
  // --------------------------------------------------

  output_a.begin(OUTPUT_A);
  channel_a.begin(&output_a, SIGNAL_E);
  channel_a.set_ratio(40, 3200);
  channel_a.invert_output();

  // --------------------------------------------------
  // OUTPUT B = X AXIS
  // --------------------------------------------------

  output_b.begin(OUTPUT_B);
  channel_b.begin(&output_b, SIGNAL_E);
  channel_b.set_ratio(40, 3200);
  channel_b.invert_output();

  // --------------------------------------------------
  // OUTPUT C = SPIN
  // --------------------------------------------------

  output_c.begin(OUTPUT_C);
  channel_c.begin(&output_c, SIGNAL_E);
  channel_c.set_ratio(360, 3200);

  // --------------------------------------------------
  // ENABLE DRIVERS BUT KEEP MOTION OFF
  // --------------------------------------------------

  enable_drivers();

  // --------------------------------------------------
  // GENERATORS
  // --------------------------------------------------

  velocityGenA.begin();
  positionGenB.begin();
  velocityGenC.begin();

  velocityGenA.output.map(&channel_a.input_target_position);
  positionGenB.output.map(&channel_b.input_target_position);
  velocityGenC.output.map(&channel_c.input_target_position);

  stop_all_motion();

  // --------------------------------------------------
  // RPC
  // --------------------------------------------------

  rpc.begin();

  rpc.enroll("set_speed_a", set_speed_a);
  rpc.enroll("set_speed_b", set_speed_b);
  rpc.enroll("set_spin_speed_c", set_spin_speed_c);

  last_time = millis();

  dance_start();

  Serial.println("System ready. Press D1 button to toggle ON/OFF.");
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {

  overhead_delay.periodic_call(&report_overhead, 500);

  read_button();
  read_controls();

  if (system_on) {
    auto_bounce_motion();
  } else {
    stop_all_motion();
    print_off_status();
  }

  dance_loop();
}

// --------------------------------------------------
// BUTTON
// --------------------------------------------------

void read_button() {

  bool reading = digitalRead(IO_D1);

  if (reading != last_button_reading) {
    last_debounce_time = millis();
  }

  if ((millis() - last_debounce_time) > debounce_delay) {

    if (reading != stable_button_state) {

      stable_button_state = reading;

      // INPUT_PULLUP: pressed = LOW
      if (stable_button_state == LOW) {

        system_on = !system_on;

        if (system_on) {
          start_all_motion();
          Serial.println("BUTTON: SYSTEM ON");
        } else {
          stop_all_motion();
          Serial.println("BUTTON: SYSTEM OFF");
        }
      }
    }
  }

  last_button_reading = reading;
}

// --------------------------------------------------
// START / STOP
// --------------------------------------------------

void start_all_motion() {

  enable_drivers();

  last_time = millis();

  estimated_pos_b = 0.0;
  b_moving_to_max = true;
  b_command_sent = false;

  velocityGenC.speed_units_per_sec = spin_speed_c;
}

void stop_all_motion() {

  velocityGenA.speed_units_per_sec = 0;
  velocityGenC.speed_units_per_sec = 0;

  // B position generator cannot be "speed = 0" directly,
  // so we command it to hold current estimated position.
  positionGenB.go(estimated_pos_b, ABSOLUTE, 1);

  disable_drivers();
}

// --------------------------------------------------
// READ CONTROLS
// --------------------------------------------------

void read_controls() {

  // A range
  max_pos_a = analog_a1.read();

  if (estimated_pos_a > max_pos_a) {
    estimated_pos_a = max_pos_a;
  }

  // B range = bar length
  max_pos_b = analog_a2.read();

  if (estimated_pos_b > max_pos_b) {
    estimated_pos_b = max_pos_b;
  }

  // C spin speed
  spin_speed_c = analog_a3.read();

  // A speed encoder
  float encoder_value_a = encoder_1.read();
  float diff_a = encoder_value_a - last_encoder_value_a;

  if (diff_a != 0) {
    speed_a += diff_a * encoder_speed_step_a;
    speed_a = constrain(speed_a, min_speed_a, max_speed_a);
    last_encoder_value_a = encoder_value_a;
  }

  // B speed encoder
  float encoder_value_b = encoder_2.read();
  float diff_b = encoder_value_b - last_encoder_value_b;

  if (diff_b != 0) {
    speed_b += diff_b * encoder_speed_step_b;
    speed_b = constrain(speed_b, min_speed_b, max_speed_b);
    last_encoder_value_b = encoder_value_b;
  }
}

// --------------------------------------------------
// AUTO BOUNCE MOTION
// --------------------------------------------------

void auto_bounce_motion() {

  uint32_t now = millis();
  float dt = (now - last_time) / 1000.0;
  last_time = now;

  // --------------------------------------------------
  // A AXIS = velocity bounce
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
  // B AXIS = exact 0 <-> max_pos_b bounce
  // --------------------------------------------------

  if (!b_command_sent) {
    positionGenB.go(max_pos_b, ABSOLUTE, speed_b);
    b_moving_to_max = true;
    b_command_sent = true;
    estimated_pos_b = 0.0;
  }

  if (b_moving_to_max) {

    estimated_pos_b += speed_b * dt;

    if (estimated_pos_b >= max_pos_b) {
      estimated_pos_b = max_pos_b;
      positionGenB.go(0.0, ABSOLUTE, speed_b);
      b_moving_to_max = false;
    }

  } else {

    estimated_pos_b -= speed_b * dt;

    if (estimated_pos_b <= 0.0) {
      estimated_pos_b = 0.0;
      positionGenB.go(max_pos_b, ABSOLUTE, speed_b);
      b_moving_to_max = true;
    }
  }

  // --------------------------------------------------
  // C AXIS = continuous spin
  // --------------------------------------------------

  velocityGenC.speed_units_per_sec = spin_speed_c;

  // --------------------------------------------------
  // SERIAL PRINT
  // --------------------------------------------------

  if (millis() - last_print > 100) {

    Serial.print("System: ");
    Serial.print(system_on ? "ON" : "OFF");

    Serial.print(" | D1 raw: ");
    Serial.print(digitalRead(IO_D1));

    Serial.print(" | A Range: ");
    Serial.print(max_pos_a, 2);

    Serial.print(" | A Speed: ");
    Serial.print(speed_a, 2);

    Serial.print(" | B Range: ");
    Serial.print(max_pos_b, 2);

    Serial.print(" | B Speed: ");
    Serial.print(speed_b, 2);

    Serial.print(" | C Spin: ");
    Serial.print(spin_speed_c, 2);

    Serial.print(" | A Pos: ");
    Serial.print(estimated_pos_a, 2);

    Serial.print(" | B Pos: ");
    Serial.println(estimated_pos_b, 2);

    last_print = millis();
  }
}

// --------------------------------------------------
// PRINT OFF STATUS
// --------------------------------------------------

void print_off_status() {

  if (millis() - last_print > 500) {

    Serial.print("System: OFF");

    Serial.print(" | D1 raw: ");
    Serial.println(digitalRead(IO_D1));

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