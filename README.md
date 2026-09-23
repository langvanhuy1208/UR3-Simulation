# UR3-Simulation
# 1. Chuyển đến thư mục gốc của ROS 2 Workspace
cd ~/ros2_ws

# 2. Biên dịch riêng package ur_letter_writer
colcon build --packages-select ur_letter_writer --symlink-install

# 3. Nạp biến môi trường của Workspace vào Terminal
source install/setup.bash

# 4. Khởi chạy 
ros2 launch ur_letter_writer draw_letter.launch.py
