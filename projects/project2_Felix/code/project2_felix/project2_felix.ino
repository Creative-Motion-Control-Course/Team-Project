/*
Step-A-Sketch: A digital etch-a-sketch

Example project for the Stepdance control system.

A part of the Mixing Metaphors Project

// (c) 2025 Ilan Moyer, Jennifer Jacobs, Devon Frost
*/

#define module_driver  // tells compiler we're using the Stepdance Driver Module PCB \
                       // This configures pin assignments for the Teensy 4.1

#include "stepdance.hpp"  // Import the stepdance library

OutputPort output_a;  // x motor (the yellow one)
OutputPort output_b;  // y motor (the gear/rack)
OutputPort output_c;  // servo axis
OutputPort output_d;  // Z axis

Channel channel_x;  // X axis
Channel channel_y;  // Y Axis
Channel channel_r;  // servo
Channel channel_z;  // Z axis

// -- Define Encoders --
Encoder encoder_1;
Encoder encoder_2;

// -- Define Input Button --
Button button_d1;

// -- Position Generators
PositionGenerator position_gen_servo;
PositionGenerator position_gen_z;

// -- Scaling filter for attempted coordinated servo/ z movement --
ScalingFilter1D rotation_gen;


RPC rpc;  //RPC Interface

void setup() {
  output_a.begin(OUTPUT_A);
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);
  output_d.begin(OUTPUT_D);

  enable_drivers();

  channel_x.begin(&output_a, SIGNAL_E);
  channel_x.set_ratio(1, 80);

  channel_y.begin(&output_b, SIGNAL_E);
  channel_y.set_ratio(1, 80);  //RATIO NOT CORRECT, NEEDS TO BE UPDATED BASED ON MECHANISM

  channel_r.begin(&output_c, SIGNAL_E);  //servo motor, so we use a long pulse width
  channel_r.set_ratio(1, 1);             //straight step pass-thru.

  channel_z.begin(&output_d, SIGNAL_E);
  channel_z.set_ratio(1, 400);  //maybe not correct given new motor but probably close enough for now.

  // -- Configure and start the encoders --
  encoder_1.begin(ENCODER_1);  
  encoder_1.set_ratio(24, 2400);
  encoder_1.output.map(&channel_x.input_target_position);

  encoder_2.begin(ENCODER_2);
  encoder_2.set_ratio(24, 2400);
  encoder_2.output.map(&channel_y.input_target_position);

  // -- Configure Button --
  // button_d1.begin(IO_D1, INPUT_PULLDOWN);
  // button_d1.set_mode(BUTTON_MODE_TOGGLE);

  // -- Configure Position Generator --
  position_gen_servo.output.map(&channel_r.input_target_position);
  position_gen_servo.begin();
  position_gen_z.output.map(&channel_z.input_target_position);
  position_gen_z.begin();

  rotation_gen.begin();
  rotation_gen.set_ratio(1420, 200);  // maxium z axis is 250 mm
                                      // 1500 of the servo is 190 degrees
  rotation_gen.input.map(&channel_z.input_target_position);
  rotation_gen.output.map(&channel_r.input_target_position);

  rpc.begin();
  rpc.enroll("disable_z_mapping", disable_z_mapping);
  rpc.enroll("enable_z_mapping", enable_z_mapping);
  rpc.enroll("down", down);
  rpc.enroll("mid", mid);
  rpc.enroll("up", up);
  // rpc.enroll("minusTest", minusTest);
  // rpc.enroll("positiveTest", positiveTest);

  // -- Start the stepdance library --
  // This activates the system.
  dance_start();
  disable_z_mapping();
}

LoopDelay overhead_delay;

void loop() {
  overhead_delay.periodic_call(&report_overhead, 500);

  dance_loop();  // Stepdance loop provides convenience functions, and should be called at the end of the main loop
}

// // {"name":"minusTest"}
// void minusTest() {
//   position_gen_servo.go(-710, ABSOLUTE, 1000);
// }

// // {"name":"positiveTest"}
// void positiveTest() {
//   position_gen_servo.go(710, ABSOLUTE, 1000);
// }

void report_overhead() {
  Serial.println(stepdance_get_cpu_usage(), 4);
  Serial.println("encoder2");
  Serial.println(encoder_2.output.read(ABSOLUTE));
}

// {"name":"disable_z_mapping"}
void disable_z_mapping() {
  rotation_gen.output.disable();
}
// {"name":"enable_z_mapping"}
void enable_z_mapping() {
  rotation_gen.output.enable();
  position_gen_servo.go(710, ABSOLUTE, 100);  // go to the bottom, and then assemble the thing
}

// {"name":"down"}
void down() {
  // rotation_gen.output.disable();
  // position_gen_servo.go(710, ABSOLUTE, 100);
  position_gen_z.go(0, ABSOLUTE, 10);  // positive is down for z, 10 is a great speed
  // rotation_gen.output.enable();
}

// {"name":"mid"}
void mid() {
  // rotation_gen.output.disable();
  // position_gen_servo.go(0, ABSOLUTE, 100);
  position_gen_z.go(-100, ABSOLUTE, 10);  // positive is down for z, 10 is a great speed
  // rotation_gen.output.enable();
}

// {"name":"up"}
void up() {
  // rotation_gen.output.disable();
  // position_gen_servo.go(-710, ABSOLUTE, 100);
  position_gen_z.go(-200, ABSOLUTE, 10);  // positive is down for z, 10 is a great speed, 180 is basically 25cm for the servo
  // rotation_gen.output.enable();
}