import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32, Bool


class ServoAnglePublisher(Node):
    def __init__(self):
        super().__init__('servo_angle_publisher')
        self.angle_publisher = self.create_publisher(Int32, '/servo_angle', 10)
        self.enable_publisher = self.create_publisher(Bool, '/servo_enable', 10)


        self.angle = 0
        self.enabled = True

        self.timer = self.create_timer(1.0, self.publish_commands)
        

    def publish_commands(self):
        angle_msg = Int32()
        angle_msg.data = self.angle

        enabled_msg = Bool()
        enabled_msg.data = self.enabled

        self.angle_publisher.publish(angle_msg)
        self.enable_publisher.publish(enabled_msg)

        self.get_logger().info(
            f'Published angle={angle_msg.data}, enabled={enabled_msg.data}'
        )

        self.angle += 30
        if self.angle > 180:
            self.angle = 0
        





def main(args=None):
    rclpy.init(args=args)
    node = ServoAnglePublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()