/*
Triple Stepper + Continuous Rotation Servo

OUTPUT_A = Stepper A / bounce
OUTPUT_B = Stepper B / exact 0 <-> range bounce
OUTPUT_C = Stepper C / continuous spin
OUTPUT_D = Continuous rotation servo

ENCODER_1 = A speed
ENCODER_2 = B speed

IO_A1 = A range
IO_A2 = B range

IO_D1 = ALL ON/OFF
IO_D2 = Servo ON/OFF only when ALL is ON
*/

#define module_driver
#include "stepdance.hpp"

// INPUTS
Encoder encoder_1;
Encoder encoder_2;

AnalogInput analog_a1;
AnalogInput analog_a2;

Button button_d1;
Button button_d2;

// OUTPUTS
OutputPort output_a;
OutputPort output_b;
OutputPort output_c;
OutputPort output_d;

// CHANNELS
Channel channel_a;
Channel channel_b;
Channel channel_c;
Channel channel_servo;

// GENERATORS
VelocityGenerator velocityGenA;
PositionGenerator positionGenB;
VelocityGenerator velocityGenC;
PositionGenerator servoGen;

// SETTINGS
float min_pos = 0.0;

float max_pos_a = 100.0;
float max_pos_b = 100.0;

float speed_a = 50.0;
float speed_b = 50.0;

float min_range_a = 10.0;
float max_range_a = 300.0;

float min_range_b = 10.0;
float max_range_b = 300.0;

float min_speed_a = 5.0;
float max_speed_a = 200.0;

float min_speed_b = 5.0;
float max_speed_b = 200.0;

float encoder_speed_step_a = 1.0;
float encoder_speed_step_b = 1.0;

float last_encoder_value_a = 0.0;
float last_encoder_value_b = 0.0;

// OUTPUT_C spin stepper speed
float spin_speed_c = 150.0;

// SERVO
// If servo does not fully stop, tune this value: 88, 89, 90, 91, 92...
// If your original -90 worked better, set this to -90.0.
float servo_stop_value = -90.0;
float servo_run_value = 180.0;
float servo_move_speed = 2000.0;

// POSITIONS
float estimated_pos_a = 0.0;
float estimated_pos_b = 0.0;

int direction_a = 1;

bool b_moving_to_max = true;
bool b_command_sent = false;

// STATES
bool all_on = false;
bool servo_on = false;

// TIMING
uint32_t last_time = 0;
uint32_t last_print = 0;

LoopDelay overhead_delay;

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(115200);

  // POTENTIOMETERS
  analog_a1.set_floor(min_range_a, 25);
  analog_a1.set_ceiling(max_range_a, 1020);
  analog_a1.begin(IO_A1);

  analog_a2.set_floor(min_range_b, 25);
  analog_a2.set_ceiling(max_range_b, 1020);
  analog_a2.begin(IO_A2);

  // ENCODERS
  encoder_1.begin(ENCODER_1);
  encoder_1.set_ratio(1.0, 1.0);

  encoder_2.begin(ENCODER_2);
  encoder_2.set_ratio(1.0, 1.0);

  last_encoder_value_a = encoder_1.read();
  last_encoder_value_b = encoder_2.read();

  // BUTTONS
  button_d1.begin(IO_D1, INPUT_PULLDOWN);
  button_d1.set_mode(BUTTON_MODE_STANDARD);
  button_d1.set_callback_on_press(&toggle_all);

  button_d2.begin(IO_D2, INPUT_PULLDOWN);
  button_d2.set_mode(BUTTON_MODE_STANDARD);
  button_d2.set_callback_on_press(&toggle_servo);

  // OUTPUT_A
  output_a.begin(OUTPUT_A);
  channel_a.begin(&output_a, SIGNAL_E);
  channel_a.set_ratio(40, 3200);
  channel_a.invert_output();

  // OUTPUT_B
  output_b.begin(OUTPUT_B);
  channel_b.begin(&output_b, SIGNAL_E);
  channel_b.set_ratio(40, 3200);
  channel_b.invert_output();

  // OUTPUT_C = SPIN STEPPER
  output_c.begin(OUTPUT_C);
  channel_c.begin(&output_c, SIGNAL_E);
  channel_c.set_ratio(360, 3200);

  // OUTPUT_D = SERVO
  output_d.begin(OUTPUT_D);
  channel_servo.begin(&output_d, SIGNAL_E);
  channel_servo.set_ratio(1, 1);

  enable_drivers();

  // GENERATORS
  velocityGenA.begin();
  positionGenB.begin();
  velocityGenC.begin();
  servoGen.begin();

  velocityGenA.output.map(&channel_a.input_target_position);
  positionGenB.output.map(&channel_b.input_target_position);
  velocityGenC.output.map(&channel_c.input_target_position);
  servoGen.output.map(&channel_servo.input_target_position);

  last_time = millis();

  dance_start();

  stop_all_motion();
  force_servo_off();

  Serial.println("Ready");
  Serial.println("D1 = ALL ON/OFF");
  Serial.println("D2 = SERVO ON/OFF");
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {

  overhead_delay.periodic_call(&report_overhead, 500);

  read_controls();

  if (all_on) {
    auto_bounce_motion();
  } else {
    stop_all_motion();
  }

  // IMPORTANT:
  // If ALL is OFF, servo is forced OFF no matter what.
  if (!all_on) {
    servo_on = false;
    force_servo_off();
  } else {
    if (servo_on) {
      run_servo();
    } else {
      force_servo_off();
    }
  }

  print_status();

  dance_loop();
}

// --------------------------------------------------
// BUTTONS
// --------------------------------------------------

void toggle_all() {

  all_on = !all_on;

  if (all_on) {

    start_all_motion();

    servo_on = true;
    run_servo();

    Serial.println("D1: ALL ON + SERVO ON");

  } else {

    all_on = false;
    servo_on = false;

    stop_all_motion();
    force_servo_off();

    Serial.println("D1: ALL OFF + SERVO OFF");
  }
}

void toggle_servo() {

  if (!all_on) {
    servo_on = false;
    force_servo_off();
    Serial.println("D2 ignored: ALL OFF");
    return;
  }

  servo_on = !servo_on;

  if (servo_on) {
    run_servo();
    Serial.println("D2: SERVO ON");
  } else {
    force_servo_off();
    Serial.println("D2: SERVO OFF");
  }
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

  positionGenB.go(estimated_pos_b, ABSOLUTE, 1);
}

// --------------------------------------------------
// SERVO
// --------------------------------------------------

void run_servo() {
  servoGen.go(servo_run_value, ABSOLUTE, servo_move_speed);
}

void force_servo_off() {
  servoGen.go(servo_stop_value, ABSOLUTE, servo_move_speed);
}

// --------------------------------------------------
// READ CONTROLS
// --------------------------------------------------

void read_controls() {

  max_pos_a = analog_a1.read();

  if (estimated_pos_a > max_pos_a) {
    estimated_pos_a = max_pos_a;
  }

  max_pos_b = analog_a2.read();

  if (estimated_pos_b > max_pos_b) {
    estimated_pos_b = max_pos_b;
  }

  float encoder_value_a = encoder_1.read();
  float diff_a = encoder_value_a - last_encoder_value_a;

  if (diff_a != 0) {
    speed_a += diff_a * encoder_speed_step_a;
    speed_a = constrain(speed_a, min_speed_a, max_speed_a);
    last_encoder_value_a = encoder_value_a;
  }

  float encoder_value_b = encoder_2.read();
  float diff_b = encoder_value_b - last_encoder_value_b;

  if (diff_b != 0) {
    speed_b += diff_b * encoder_speed_step_b;
    speed_b = constrain(speed_b, min_speed_b, max_speed_b);
    last_encoder_value_b = encoder_value_b;
  }
}

// --------------------------------------------------
// MOTION
// --------------------------------------------------

void auto_bounce_motion() {

  uint32_t now = millis();
  float dt = (now - last_time) / 1000.0;
  last_time = now;

  // A bounce
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

  // B exact 0 <-> max_pos_b
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

  // C continuous stepper spin
  velocityGenC.speed_units_per_sec = spin_speed_c;
}

// --------------------------------------------------
// SERIAL PRINT
// --------------------------------------------------

void print_status() {

  if (millis() - last_print > 300) {

    Serial.print("ALL: ");
    Serial.print(all_on ? "ON" : "OFF");

    Serial.print(" | Servo: ");
    Serial.print(servo_on ? "ON" : "OFF");

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

    Serial.print(" | Servo Stop: ");
    Serial.print(servo_stop_value, 2);

    Serial.print(" | A Pos: ");
    Serial.print(estimated_pos_a, 2);

    Serial.print(" | B Pos: ");
    Serial.println(estimated_pos_b, 2);

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