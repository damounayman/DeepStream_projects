#pragma once

#include <librealsense2/rs.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class GstRgbdServer {
public:
    GstRgbdServer();
    ~GstRgbdServer();

    void startStreaming();
    void stopStreaming();

private:
    static GstFlowReturn newSample(GstElement* appsrc, gpointer user_data);
    void initializePipeline();
    void cleanup();

    rs2::pipeline pipeline;
    GstElement* gstPipeline;
    GstElement* appsrc;
    GstElement* videosink;
    GMainLoop* mainLoop;

    // Forward declaration of startMainLoop
    static void startMainLoop(GstRgbdServer* server);
};