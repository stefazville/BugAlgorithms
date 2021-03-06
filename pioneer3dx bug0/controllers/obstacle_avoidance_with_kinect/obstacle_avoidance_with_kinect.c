/*
 * File:         obstacle_avoidance_with_kinect.c
 * Date:         August 24th, 2011
 * Description:  A Braitenberg-like controller moving a Pioneer 3-DX equipped with a kinect.
 * Author:       can
 *
 * Copyright (c) 2011 Cyberbotics - www.cyberbotics.com
 */

#include <webots/robot.h>
#include <webots/servo.h> 
#include <webots/camera.h>
#include <webots/distance_sensor.h>
#include <webots/gps.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAX_SPEED 10.0
#define CRUISING_SPEED 7.5
#define TOLERANCE -0.1
#define OBSTACLE_THRESHOLD 0.5
#define SLOWDOWN_FACTOR 0.5

static WbDeviceTag front, center;
static WbDeviceTag leftWheel, rightWheel;
static WbDeviceTag kinect;
const float* kinectValues;
static int time_step = 64;
const double *point1;
const double *point2;
static WbDeviceTag distance_sensor[8];
double distance_value[8];
int main()
{
    wb_robot_init();
    time_step  = wb_robot_get_basic_time_step();
    leftWheel  = wb_robot_get_device("leftWheel");
    rightWheel = wb_robot_get_device("rightWheel");
    kinect     = wb_robot_get_device("kinect");
    wb_camera_enable(kinect, time_step);
    int kinectWidth = wb_camera_get_width(kinect);
    int kinectHeight = wb_camera_get_height(kinect);
    int halfWidth   = kinectWidth/2;
    int viewHeight  = kinectHeight/2 + 10;
    double maxRange = wb_camera_get_max_range(kinect);
    double rangeThreshold = 0.7;
    double invMaxRangeTimesWidth = 1.0 / (maxRange * kinectWidth);
    static float targetX=0;
    static float targetZ=4.684;

  front = wb_robot_get_device("frontGps");
  center = wb_robot_get_device("centerGps");
  wb_gps_enable(front,time_step);
  wb_gps_enable(center,time_step);
  // set servos' positions
  wb_servo_set_position(leftWheel,INFINITY);
  wb_servo_set_position(rightWheel,INFINITY);
  // set speeds 
  wb_servo_set_velocity(leftWheel, 0.0);
  wb_servo_set_velocity(rightWheel, 0.0);
  //perform one control loop
  wb_robot_step(time_step);
  distance_sensor[0] = wb_robot_get_device("so0");
  distance_sensor[1] = wb_robot_get_device("so1");
  distance_sensor[2] = wb_robot_get_device("so2");
  distance_sensor[3] = wb_robot_get_device("so3");
  distance_sensor[4] = wb_robot_get_device("so4");
  distance_sensor[5] = wb_robot_get_device("so5");
  distance_sensor[6] = wb_robot_get_device("so6");
  distance_sensor[7] = wb_robot_get_device("so7");
  wb_distance_sensor_enable(distance_sensor[0],time_step);
  wb_distance_sensor_enable(distance_sensor[1],time_step);
  wb_distance_sensor_enable(distance_sensor[2],time_step);
  wb_distance_sensor_enable(distance_sensor[3],time_step);
  wb_distance_sensor_enable(distance_sensor[4],time_step);
  wb_distance_sensor_enable(distance_sensor[5],time_step);
  wb_distance_sensor_enable(distance_sensor[6],time_step);
  wb_distance_sensor_enable(distance_sensor[7],time_step);
  // init dynamic variables
  double leftObstacle = 0.0, rightObstacle = 0.0, obstacle = 0.0;
  double deltaObstacle = 0.0;
  double leftSpeed = CRUISING_SPEED, rightSpeed = CRUISING_SPEED;
  double speedFactor  = 1.0;
  float targetAngle;
  float currentAngle;
  float value = 0.0;
  int i = 0;
  double sideObstacle = 0.0;
  while (1) {
    
    for(i=0; i<8; i++){
      distance_value[i] = wb_distance_sensor_get_value(distance_sensor[i]);
    }
    for(i=0; i<8; i++){
      printf(" %f",distance_value[i]);
    }
    sideObstacle = distance_value[0] + distance_value[1] + distance_value[6] + distance_value[7];
    
    
    
    kinectValues = (float*) wb_camera_get_range_image(kinect);
    for (i = 0; i < halfWidth; i++) {
      value = wb_camera_range_image_get_depth(kinectValues, kinectWidth, i, viewHeight);
      if(value < rangeThreshold) { 
        leftObstacle += value;
      }
      value = wb_camera_range_image_get_depth(kinectValues, kinectWidth, kinectWidth - i, viewHeight);
      if(value < rangeThreshold) {
        rightObstacle += value;
      }
    }
    obstacle  =  leftObstacle + rightObstacle;
    point1 = (float*) wb_gps_get_values(front);
    point2 = (float*) wb_gps_get_values(center);
    targetAngle = atan2(targetZ-point1[2],targetX-point1[0]);
    currentAngle = atan2(point2[2]-point1[2],point2[0]-point1[0]);
    
    //if reach the target, stop
    if(sqrt(pow(targetZ-point2[2],2) + pow(targetX-point2[0],2))<0.6)
    {
      wb_servo_set_velocity(leftWheel,  0);
      wb_servo_set_velocity(rightWheel, 0);
      printf("!!! Target reached !!!");
      break;
    }
    
    
    if(obstacle > 9){
      printf("\n 1 \n");
      obstacle = 1.0 - obstacle * invMaxRangeTimesWidth;// compute the relevant overall quantity of obstacle
      speedFactor = (obstacle > OBSTACLE_THRESHOLD) ? 0.0 : SLOWDOWN_FACTOR;
      deltaObstacle = - (leftObstacle - rightObstacle) * invMaxRangeTimesWidth;
      if(deltaObstacle > TOLERANCE){
        leftSpeed    =               CRUISING_SPEED;
        rightSpeed   = speedFactor * CRUISING_SPEED;   
      }
      else {
        leftSpeed    = speedFactor *  CRUISING_SPEED;
        rightSpeed   =                CRUISING_SPEED;
      }
    }else if(sideObstacle < 0.1  &&fabs(targetAngle-currentAngle)>0.3){
      printf("\n case: 2 \n");
      if(targetAngle < currentAngle){
        leftSpeed    -=0.5;
        rightSpeed   +=0.5;   
      }
      else {
        leftSpeed    +=0.5;
        rightSpeed   -=0.5;
      }
      printf("x1:%2f z1:%2f; x2:%2f z2:%2f ", point1[0],point1[2],point2[0],point2[2]);  
    }else {
      printf("3 \n");
       leftSpeed    = CRUISING_SPEED;
       rightSpeed   = CRUISING_SPEED;
    }
    // set speeds
    wb_servo_set_velocity(leftWheel,  leftSpeed);
    wb_servo_set_velocity(rightWheel, rightSpeed);
    printf("%2f, %2f \n",leftObstacle,rightObstacle);
    // run simulation
    wb_robot_step(time_step);
    leftObstacle  = 0.0;
    rightObstacle = 0.0;
  }
}
