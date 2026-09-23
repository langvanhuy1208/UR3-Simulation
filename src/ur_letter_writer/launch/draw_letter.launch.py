import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    ur_gz_sim_dir = get_package_share_directory('ur_simulation_gz')
    ur_letter_writer_dir = get_package_share_directory('ur_letter_writer')

    # Đường dẫn đến file config RViz đã lưu
    rviz_config_file = os.path.join(ur_letter_writer_dir, 'config', 'draw_letter.rviz')

    # Khởi chạy mô phỏng (Tắt RViz mặc định)
    ur_sim_moveit_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ur_gz_sim_dir, 'launch', 'ur_sim_moveit.launch.py')
        ),
        launch_arguments={
            'ur_type': 'ur3e',
            'launch_rviz': 'false'
        }.items()
    )

    # Tự mở RViz2 kèm cấu hình có sẵn Marker
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_config_file]
    )

    # Node điều khiển vẽ chữ
    draw_node = Node(
        package='ur_letter_writer',
        executable='draw_letter_node',
        output='screen'
    )

    # Chờ 12 giây cho Gazebo và RViz2 load xong mới bắt đầu vẽ
    delayed_draw_node = TimerAction(
        period=12.0,
        actions=[draw_node]
    )

    return LaunchDescription([
        ur_sim_moveit_launch,
        rviz_node,
        delayed_draw_node
    ])
