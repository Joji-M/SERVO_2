#include <Arduino.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

#define LED_PIN 2

rcl_subscription_t subscriber;
std_msgs__msg__Int32 msg;

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

volatile int received_angle = 0;
volatile bool received_new_angle = false;
const int SERVO_PIN = 18;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 50;
const int PWM_RESOLUTION = 16;

const int MAX_DUTY = (1 << PWM_RESOLUTION) -1;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if ((temp_rc != RCL_RET_OK)) { error_loop(); } }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; (void)temp_rc; }

void error_loop() {
  while (1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}



int pulseWidthUsToDuty(int pulseWidthUs);
int angleToPulseWidth(int angle);

void subscription_callback(const void * msgin) {
  const std_msgs__msg__Int32 * incoming_msg = (const std_msgs__msg__Int32 *)msgin;


  received_angle = incoming_msg->data;
  int pulseWidthUs = angleToPulseWidth(received_angle);
  int duty = pulseWidthUsToDuty(pulseWidthUs);
  ledcWrite(PWM_CHANNEL, duty);

  received_new_angle = true;




  // Do NOT Serial.println() here when using serial micro-ROS transport.
  // Serial is being used for micro-ROS communication.
}





int pulseWidthUsToDuty(int pulseWidthUs){
  const int periodUs = 20000;
  return (pulseWidthUs * MAX_DUTY) / periodUs;

}

int angleToPulseWidth(int angle){
  //500-2500us pulses range. 
  const int maxAngle = 180;
  const int minAngle = 0;
  const int maxPulse = 2500;
  const int minPulse = 500;
  if(angle < minAngle){
    angle = minAngle;
  }
  if(angle > maxAngle){
    angle = maxAngle;
  }

  //0-2000

  return (int)(((angle * 1.0) / maxAngle) * (maxPulse- minPulse) + 500);

}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ledcSetup(PWM_CHANNEL, PWM_FREQ,PWM_RESOLUTION);
  ledcAttachPin(SERVO_PIN,PWM_CHANNEL);

  Serial.begin(115200);
  delay(2000);

  set_microros_serial_transports(Serial);
  while (rmw_uros_ping_agent(1000, 1) != RMW_RET_OK) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(500);
  }

  allocator = rcl_get_default_allocator();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  RCCHECK(rclc_node_init_default(
    &node,
    "esp32_servo_subscriber",
    "",
    &support
  ));

  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "/servo_angle"
  ));

  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "/servo_enable"
  ));


  RCCHECK(rclc_executor_init(
    &executor,
    &support.context,
    2,
    &allocator
  ));

  RCCHECK(rclc_executor_add_subscription(
    &executor,
    &subscriber,
    &msg,
    &subscription_callback,
    ON_NEW_DATA
  ));
  
  RCCHECK(rclc_executor_add_subscription(
    &executor,
    &subscriber,
    &msg,
    &subscription_callback,
    ON_NEW_DATA
  ));
}

void loop() {
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));

  if (received_new_angle) {
    received_new_angle = false;

    // Simple visible proof that a message arrived.
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  delay(10);
}
