from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='mowgli_unicore_gnss',
            executable='um982_node',
            name='um982_node',
            output='screen',
            parameters=[
                PathJoinSubstitution([
                    FindPackageShare('mowgli_unicore_gnss'),
                    'config',
                    'um982.yaml',
                ])
            ],
        )
    ])
