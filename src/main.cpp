/***************************************************************************
 *                                                                         *
 *   This file is part of the Icp Library,                                 *
 *                                                                         *
 *   Copyright (C) 2014 by Arnaud TANGUY <arn.tanguy@NOSPAM.gmail.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <glog/logging.h>
#include <pcl/common/transforms.h>
#include <pcl/visualization/cloud_viewer.h>
#include <sophus/se3.hpp>
#include "icp.hpp"

/**
 * \brief Creates a transformation matrix:
 * [ R3x3, T3x1]
 * [ 0   , 1   ]
 *
 * \params
 *   tx, ty, tz : translation
 *   rx, ry, rz : rotation around axis x, y and z (in radian)
 *
 * \returns
 * The transformation matrix
 */
template<typename T>
Eigen::Matrix<T, 4, 4> createTransformationMatrix(T tx, T ty, T tz,
                                                  T rx, T ry, T rz) {
  typedef Eigen::Transform<T, 3, Eigen::Affine> Affine3;
  typedef Eigen::Matrix<T, 3, 1> Vector3;

  Affine3 r =
      Affine3(Eigen::AngleAxis<T>(rx, Vector3::UnitX()))
      * Affine3(Eigen::AngleAxis<T>(ry,  Vector3::UnitY()))
      * Affine3(Eigen::AngleAxis<T>(rz, Vector3::UnitZ()));
  Affine3 t(Eigen::Translation<T, 3>(Vector3(tx, ty, tz)));
  Eigen::Matrix<T, 4, 4> m = (t * r).matrix();
  return m;
}

int main(int argc, char *argv[]) {
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "Starting ICP program";

  /*
    Loads test point cloud
  */
  LOG(INFO) << "Loading Model pointcloud";
  pcl::PointCloud<pcl::PointXYZ>::Ptr modelCloud(
      new pcl::PointCloud<pcl::PointXYZ>());
  if (pcl::io::loadPCDFile<pcl::PointXYZ> ("../models/teapot.pcd",
                                           *modelCloud) == -1) {
    PCL_ERROR("Couldn't read file ../models/teapot.pcd \n");
    return (-1);
  }
  LOG(INFO) << "Model Point cloud has " << modelCloud->points.size()
            << " points";



  /*
    Creating a second transformed pointcloud
  */
  Eigen::Matrix4f transformation
      = createTransformationMatrix(0.f,
                                   0.05f,
                                   0.f,
                                   static_cast<float>(M_PI)/200.f,
                                   static_cast<float>(M_PI)/200.f,
                                   0.f);
  LOG(INFO) << "Transformation:\n"<< transformation;

  // Executing the transformation
  pcl::PointCloud<pcl::PointXYZ>::Ptr dataCloud
      (new pcl::PointCloud<pcl::PointXYZ>());
  // Generates a data point cloud to be matched against the model
  pcl::transformPointCloud(*modelCloud, *dataCloud, transformation);



  /**
     Initial guess
  **/
  Eigen::Matrix4f initialGuessTransformation = Eigen::Matrix4f::Identity();
  Sophus::SE3Group<float> initial_guess_SO3(initialGuessTransformation);
  Sophus::SE3Group<float>::Tangent initial_guess = initial_guess_SO3.log();


  /*
    Define parameters for the ICP
  */
  icp::IcpParametersf icp_param;
  icp_param.lambda = 0.1;
  icp_param.max_iter = 100;
  icp_param.min_variation = 10e-5;
  icp_param.initial_guess = initial_guess;
  LOG(INFO) << "ICP Parameters:\n" << icp_param;

  icp::Icp<float, float, float> icp_algorithm;
  icp_algorithm.setParameters(icp_param);
  icp_algorithm.setModelPointModelCloud(modelCloud);
  icp_algorithm.setDataPointCloud(modelCloud);


  /**
   * Visualize
   **/
  LOG(INFO) << "\nPoint cloud colors :  white  = original point cloud\n"
      "                       red  = transformed point cloud\n";
  pcl::visualization::PCLVisualizer viewer("Matrix transformation example");

  // Define R,G,B colors for the point cloud
  pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
      source_cloud_color_handler(modelCloud, 255, 255, 255);
  // We add the point cloud to the viewer and pass the color handler
  viewer.addPointCloud(modelCloud,
                       source_cloud_color_handler, "original_cloud");

  pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
      transformed_cloud_color_handler(dataCloud, 230, 20, 20);  // Red
  viewer.addPointCloud(dataCloud, transformed_cloud_color_handler,
                       "transformed_cloud");

  viewer.addCoordinateSystem(1.0, "cloud", 0);
  // Setting background to a dark grey
  viewer.setBackgroundColor(0.05, 0.05, 0.05, 0);
  viewer.setPointCloudRenderingProperties(
      pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 2, "original_cloud");
  viewer.setPointCloudRenderingProperties(
      pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 2, "transformed_cloud");

  // Display the visualiser until 'q' key is pressed
  while (!viewer.wasStopped()) {
    viewer.spinOnce();
  }

  return 0;
}
