#pragma once

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

#include <gst/rtsp-server/rtsp-server.h>
#include <gst/rtsp/gstrtspconnection.h>

class GstRgbdServer {
public:
  GstRgbdServer();
  ~GstRgbdServer();

  void stream();
  void stopStreaming();
  void update();
  void onNeedData(GstElement *element, guint size);

private:
  void initializeRealsense();
  rs2::align alignmentFilter_{RS2_STREAM_COLOR};
  rs2::pipeline rsPipeline_;
  rs2::config rsPipelineConfig_;
  rs2::frame_queue imuQueue_;
  rs2::frame_queue frameQueue_;
  rs2::depth_sensor *depthSensor_;
  rs2::device device_;
  double scale_;
  rs2::threshold_filter threshold_filter_;

  int32_t fps_{30};
  int width_ = 1280;
  int height_ = 720;

  rs2::frame colorFrame_;

  GMainLoop *gsLoop_;
  GstRTSPServer *gsServer_;
  GstRTSPMountPoints *gsMounts_;
  GstRTSPMediaFactory *gsFactory_;
  GOptionContext *gsOptctx_;
  GError *gsError_ = NULL;

  const gchar *port = (char *)"8554";
  const gchar *host = (char *)"127.0.0.1";
};