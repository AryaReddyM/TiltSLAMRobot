from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(package='tiltslam_base', executable='esp_comms', output='screen', respawn=True),
        Node(package='tiltslam_base', executable='teleop', output='screen', respawn=True),
    ])