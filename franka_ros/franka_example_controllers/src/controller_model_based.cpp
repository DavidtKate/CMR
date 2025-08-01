// Copyright (c) 2023 Franka Robotics GmbH
// Use of this source code is governed by the Apache-2.0 license, see LICENSE
#include <franka_example_controllers/controller_model_based.h>

#include <cmath>

#include <controller_interface/controller_base.h>
#include <hardware_interface/hardware_interface.h>
#include <hardware_interface/joint_command_interface.h>
#include <pluginlib/class_list_macros.h>
#include <ros/ros.h>

namespace franka_example_controllers {

bool ModelBasedController::init(hardware_interface::RobotHW* robot_hardware,
                                          ros::NodeHandle& node_handle) {
  position_joint_interface_ = robot_hardware->get<hardware_interface::PositionJointInterface>();
  if (position_joint_interface_ == nullptr) {
    ROS_ERROR(
        "ModelBasedController: Error getting position joint interface from hardware!");
    return false;
  }
  std::vector<std::string> joint_names;
  if (!node_handle.getParam("joint_names", joint_names)) {
    ROS_ERROR("ModelBasedController: Could not parse joint names");
  }
  if (joint_names.size() != 7) {
    ROS_ERROR_STREAM("ModelBasedController: Wrong number of joint names, got "
                     << joint_names.size() << " instead of 7 names!");
    return false;
  }
  position_joint_handles_.resize(7);
  for (size_t i = 0; i < 7; ++i) {
    try {
      position_joint_handles_[i] = position_joint_interface_->getHandle(joint_names[i]);
    } catch (const hardware_interface::HardwareInterfaceException& e) {
      ROS_ERROR_STREAM(
          "ModelBasedController: Exception getting joint handles: " << e.what());
      return false;
    }
  }

  std::array<double, 7> q_start{{0, -M_PI_4, 0, -3 * M_PI_4, 0, M_PI_2, M_PI_4}};
  for (size_t i = 0; i < q_start.size(); i++) {
    if (std::abs(position_joint_handles_[i].getPosition() - q_start[i]) > 0.1) {
      ROS_ERROR_STREAM(
          "ModelBasedController: Robot is not in the expected starting position for "
          "running this example. Run `roslaunch franka_example_controllers move_to_start.launch "
          "robot_ip:=<robot-ip> load_gripper:=<has-attached-gripper>` first.");
      return false;
    }
  }
  std::vector<double> targetPositions;
  if(!node_handle.getParam("target_joint_positions", targetPositions)) {
    ROS_ERROR("couldn't find target positions parameter");
    return false;
  }
  if(targetPositions.size() != 7) {
    ROS_ERROR_STREAM("wrong number of target positions got " << targetPositions.size() << " instead of 7");
    return false;
  }
  for (size_t i=0; i < 7 ; ++i){
    target_pose_[i] = targetPosition[i];
  }

  return true;
}

void ModelBasedController::starting(const ros::Time& /* time */) {
  for (size_t i = 0; i < 7; ++i) {
    initial_pose_[i] = position_joint_handles_[i].getPosition();
  }
  elapsed_time_ = ros::Duration(0.0);
}

void ModelBasedController::update(const ros::Time& /*time*/,
                                            const ros::Duration& period) {
  elapsed_time_ += period;

  double alpha = elapsed_time_toSec() / 5.0; // 5 is 5 seconds movement duration
  if(alpha > 1.0) {
    alpha = 1.0;
  }

//   double delta_angle = M_PI / 16 * (1 - std::cos(M_PI / 5.0 * elapsed_time_.toSec())) * 0.2;
  for (size_t i = 0; i < 7; ++i) {
    double command = (1.0  - alpha) * initial_pose_[i] = alpha * target_pose_[i];
    position_joint_handles[i].setCommand(command);

    // if (i == 4) {
    //   position_joint_handles_[i].setCommand(initial_pose_[i] - delta_angle);
    // } else {
    //   position_joint_handles_[i].setCommand(initial_pose_[i] + delta_angle);
    // }
  }
}

}  // namespace franka_example_controllers

PLUGINLIB_EXPORT_CLASS(franka_example_controllers::ModelBasedController,
                       controller_interface::ControllerBase)
