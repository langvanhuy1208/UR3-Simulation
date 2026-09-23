#include <rclcpp/rclcpp.hpp>
#include <moveit/move_group_interface/move_group_interface.h>
#include <geometry_msgs/msg/pose.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <thread>
#include <vector>

geometry_msgs::msg::Pose createPose(double x, double y, double z, const geometry_msgs::msg::Quaternion& q) {
  geometry_msgs::msg::Pose p;
  p.orientation = q;
  p.position.x = x; p.position.y = y; p.position.z = z;
  return p;
}

// Hàm thực hiện 1 nét vẽ: Publish Marker TRƯỚC rồi mới kéo tay robot
void drawStroke(
  moveit::planning_interface::MoveGroupInterface& group,
  rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr pub,
  visualization_msgs::msg::Marker& marker,
  const geometry_msgs::msg::Pose& start_pose,
  const geometry_msgs::msg::Pose& end_pose,
  double x_lift)
{
  // 1. Di chuyển trên không đến vị trí bắt đầu
  geometry_msgs::msg::Pose air_start = start_pose;
  air_start.position.x = x_lift;
  
  moveit_msgs::msg::RobotTrajectory traj;
  group.computeCartesianPath({air_start}, 0.005, 0.0, traj);
  group.execute(traj);

  // 2. Chạm tay xuống mặt phẳng vẽ
  group.computeCartesianPath({start_pose}, 0.005, 0.0, traj);
  group.execute(traj);

  // 3. PUBLISH MARKER NÉT VẼ LÊN RVIZ TRƯỚC KHI KÉO NÉT
  marker.points.push_back(start_pose.position);
  marker.points.push_back(end_pose.position);
  marker.header.stamp = rclcpp::Clock().now();
  pub->publish(marker);

  // 4. Kéo nét vẽ (Lúc này RViz đã hiển thị sẵn nét đường đi)
  group.computeCartesianPath({end_pose}, 0.005, 0.0, traj);
  group.execute(traj);

  // 5. Rút tay lên mặt phẳng an toàn
  geometry_msgs::msg::Pose air_end = end_pose;
  air_end.position.x = x_lift;
  group.computeCartesianPath({air_end}, 0.005, 0.0, traj);
  group.execute(traj);
}

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("draw_letter_node");

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);
  std::thread spinner([&executor]() { executor.spin(); });

  static const std::string PLANNING_GROUP = "ur_manipulator";
  moveit::planning_interface::MoveGroupInterface move_group(node, PLANNING_GROUP);

  move_group.setPlanningTime(10.0);
  move_group.setMaxVelocityScalingFactor(0.08); // Cho robot chạy chậm 8% để dễ quan sát
  move_group.setMaxAccelerationScalingFactor(0.08);

  auto marker_pub = node->create_publisher<visualization_msgs::msg::Marker>("/drawn_path_marker", 10);

  tf2::Quaternion q;
  q.setRPY(0, M_PI_2, 0);
  geometry_msgs::msg::Quaternion orientation = tf2::toMsg(q);

  const double X_DRAW = 0.30; 
  const double X_LIFT = 0.23; 

  visualization_msgs::msg::Marker marker;
  marker.header.frame_id = move_group.getPlanningFrame();
  marker.ns = "letter_H";
  marker.id = 0;
  marker.type = visualization_msgs::msg::Marker::LINE_LIST;
  marker.action = visualization_msgs::msg::Marker::ADD;
  marker.scale.x = 0.01; // Độ dày nét 1cm
  marker.color.r = 0.0f; marker.color.g = 0.8f; marker.color.b = 1.0f; marker.color.a = 1.0f;

  RCLCPP_INFO(node->get_logger(), "==> Đưa robot về vị trí xuất phát...");
  move_group.setPoseTarget(createPose(X_LIFT, -0.08, 0.25, orientation));
  move_group.move();

  // --- NÉT 1: THÂN DỌC TRÁI ---
  RCLCPP_INFO(node->get_logger(), "==> Bắt đầu Nét 1...");
  drawStroke(move_group, marker_pub, marker, 
             createPose(X_DRAW, -0.08, 0.25, orientation), 
             createPose(X_DRAW, -0.08, 0.45, orientation), X_LIFT);

  // --- NÉT 2: THÂN DỌC PHẢI ---
  RCLCPP_INFO(node->get_logger(), "==> Bắt đầu Nét 2...");
  drawStroke(move_group, marker_pub, marker, 
             createPose(X_DRAW, 0.08, 0.25, orientation), 
             createPose(X_DRAW, 0.08, 0.45, orientation), X_LIFT);

  // --- NÉT 3: GẠCH NGANG GIỮA ---
  RCLCPP_INFO(node->get_logger(), "==> Bắt đầu Nét 3...");
  drawStroke(move_group, marker_pub, marker, 
             createPose(X_DRAW, -0.08, 0.35, orientation), 
             createPose(X_DRAW, 0.08, 0.35, orientation), X_LIFT);

  RCLCPP_INFO(node->get_logger(), "==> HOÀN THÀNH CHỮ H!");

  // Vòng lặp giữ Marker không bị mất trên RViz
  rclcpp::Rate loop_rate(2);
  while (rclcpp::ok()) {
    marker.header.stamp = node->now();
    marker_pub->publish(marker);
    loop_rate.sleep();
  }

  spinner.join();
  return 0;
}
