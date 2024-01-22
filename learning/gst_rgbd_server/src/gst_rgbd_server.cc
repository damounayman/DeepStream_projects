#include "gst_rgbd_server/gst_rgbd_server.h"
#include <iostream>
using namespace cv;
GstRgbdServer::GstRgbdServer() { initializeRealsense(); }

GstRgbdServer::~GstRgbdServer() {}

void GstRgbdServer::initializeRealsense() {
  std::cout << "Initializing Realsense." << std::endl;

  rs2::context context;
  rs2::device_list availableDevices = context.query_devices();

  if (availableDevices.size() == 0) {
    std::cout << "No device connected, please connect a RealSense device."
              << std::endl;
  }

  rs2::device_hub hub(context);
  device_ = hub.wait_for_device();

  // Need to get the depth sensor specifically as it is the one that controls
  // the sync funciton
  depthSensor_ = new rs2::depth_sensor(device_.first<rs2::depth_sensor>());
  scale_ = depthSensor_->get_option(RS2_OPTION_DEPTH_UNITS);

  // RGB stream
  rsPipelineConfig_.enable_stream(RS2_STREAM_COLOR, width_, height_,
                                  RS2_FORMAT_BGR8, fps_);
  // Depth stream
  rsPipelineConfig_.enable_stream(RS2_STREAM_DEPTH, width_, height_,
                                  RS2_FORMAT_Z16, fps_);

  rsPipelineConfig_.enable_stream(RS2_STREAM_INFRARED, 1, width_, height_,
                                  RS2_FORMAT_Y8, fps_);
  rsPipelineConfig_.enable_stream(RS2_STREAM_INFRARED, 2, width_, height_,
                                  RS2_FORMAT_Y8, fps_);
  // Accel stream
  rsPipelineConfig_.enable_stream(RS2_STREAM_ACCEL, RS2_FORMAT_MOTION_XYZ32F);
  // Gyro stream
  rsPipelineConfig_.enable_stream(RS2_STREAM_GYRO, RS2_FORMAT_MOTION_XYZ32F);

  std::cout << "Realsense correctly initialized and ready to run." << std::endl;

  auto selection = rsPipeline_.start(rsPipelineConfig_);
  auto depth_sensor = selection.get_device().first<rs2::depth_sensor>();
  if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)) {
    depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f); // Enable emitter
  }

  GOptionEntry entries[] = {{"port", 'p', 0, G_OPTION_ARG_STRING, &port,
                             "Port to listen on( default: "
                             "8554"
                             ")",
                             "PORT"},
                            {"address", 'a', 0, G_OPTION_ARG_STRING, &host,
                             "Host address( default: "
                             "127.0.0.1"
                             ")",
                             "HOST"},
                            {NULL}};
}

void GstRgbdServer::stream() {

  // <---- Create RTSP Server pipeline
  gsLoop_ = g_main_loop_new(NULL, FALSE);

  /* create a server instance */
  gsServer_ = gst_rtsp_server_new();

  g_object_set(gsServer_, "address", host, NULL);
  g_object_set(gsServer_, "service", port, NULL);

  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  gsMounts_ = gst_rtsp_server_get_mount_points(gsServer_);

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines.
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  gsFactory_ = gst_rtsp_media_factory_new();

  std::string pipelineStr = "appsrc name=mysource ! videoconvert ! x264enc ! "
                            "rtph264pay name=pay0 pt=96";
  g_object_set(G_OBJECT(gsFactory_), "launch", pipelineStr.c_str(), NULL);

  // Set the callback for the 'need-data' signal on appsrc
  g_signal_connect(
      gsFactory_, "need-data",
      G_CALLBACK(+[](GstElement *element, guint size, gpointer user_data) {
        GstRgbdServer *server = static_cast<GstRgbdServer *>(user_data);
        server->onNeedData(element, size);
      }),
      this);

  gst_rtsp_mount_points_add_factory(gsMounts_, "/head", gsFactory_);

  // Attach the server to the default main context
  gst_rtsp_server_attach(gsServer_, NULL);

  g_main_loop_run(gsLoop_);
}

// Callback for the 'need-data' signal on appsrc
void GstRgbdServer::onNeedData(GstElement *element, guint size) {

  rs2::frameset frames;
  frames = rsPipeline_.wait_for_frames();

  colorFrame_ = frames.get_color_frame();

  // Check if the frame is valid
  if (colorFrame_) {
    const void *data = colorFrame_.get_data();
    int size = colorFrame_.get_data_size();

    // Assuming the color frame is in BGR format
    GstBuffer *buffer = gst_buffer_new_wrapped_full(
        GST_MEMORY_FLAG_READONLY, const_cast<void *>(data), size, 0, size,
        nullptr, nullptr);

    // Push the buffer to the appsrc element
    gst_app_src_push_buffer(GST_APP_SRC(element), buffer);
    GstElement  *appsrc;
    appsrc = gst_bin_get_by_name_recurse_up (GST_BIN (element), "mysrc");
  }
}

void GstRgbdServer::update() {
  rs2::frameset frames;
  frames = rsPipeline_.wait_for_frames();

  rs2::align align_to_color(RS2_STREAM_COLOR);
  frames = align_to_color.process(frames);

  // Get imu data
  if (rs2::motion_frame accel_frame =
          frames.first_or_default(RS2_STREAM_ACCEL)) {
    rs2_vector accel_sample = accel_frame.get_motion_data();
    std::cout << "Accel:" << accel_sample.x << ", " << accel_sample.y << ", "
              << accel_sample.z << std::endl;
  }
  if (rs2::motion_frame gyro_frame = frames.first_or_default(RS2_STREAM_GYRO)) {
    rs2_vector gyro_sample = gyro_frame.get_motion_data();
    std::cout << "Gyro:" << gyro_sample.x << ", " << gyro_sample.y << ", "
              << gyro_sample.z << std::endl;
  }

  // Get each frame
  colorFrame_ = frames.get_color_frame();
  rs2::depth_frame depth_frame = frames.get_depth_frame();
  rs2::video_frame ir_frame_left = frames.get_infrared_frame(1);
  rs2::video_frame ir_frame_right = frames.get_infrared_frame(2);

  // Creating OpenCV Matrix from a color image
  Mat color(Size(width_, height_), CV_8UC3, (void *)colorFrame_.get_data(),
            Mat::AUTO_STEP);
  Mat pic_right(Size(width_, height_), CV_8UC1,
                (void *)ir_frame_right.get_data());
  Mat pic_left(Size(width_, height_), CV_8UC1,
               (void *)ir_frame_left.get_data());
  Mat pic_depth(Size(width_, height_), CV_16U, (void *)depth_frame.get_data(),
                Mat::AUTO_STEP);

  // Display in a GUI
  namedWindow("Display Image");
  imshow("Display Image", color);
  imshow("Display depth", pic_depth * 30);
  imshow("Display pic_left", pic_left);
  imshow("Display pic_right", pic_right);
  waitKey(1);
}

void GstRgbdServer::stopStreaming() {}
