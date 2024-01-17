#include "gst_rgbd_server/gst_rgbd_server.h"


GstRgbdServer::GstRgbdServer() {
    gst_init(NULL, NULL);
    initializePipeline();
}

GstRgbdServer::~GstRgbdServer() {
    cleanup();
}

void GstRgbdServer::initializePipeline() {
    // Initialize RealSense pipeline
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
    pipeline.start(cfg);

    // Create GStreamer pipeline
    gstPipeline = gst_pipeline_new("rs2_gst_pipeline");
    appsrc = gst_element_factory_make("appsrc", "source");
    videosink = gst_element_factory_make("autovideosink", "videosink");

    // Set up GStreamer pipeline
    gst_bin_add(GST_BIN(gstPipeline), appsrc);
    gst_bin_add(GST_BIN(gstPipeline), videosink);

    // Link appsrc to videosink
    gst_element_link(appsrc, videosink);

    // Set up appsrc properties
    g_object_set(G_OBJECT(appsrc), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGB",
                                     "width", G_TYPE_INT, 640,
                                     "height", G_TYPE_INT, 480,
                                     "framerate", GST_TYPE_FRACTION, 30, 1,
                                     NULL),
                 "format", GST_FORMAT_TIME, NULL);

    // Set up appsrc callback for pushing frames
    g_signal_connect(appsrc, "need-data", G_CALLBACK(newSample), this);

    // Start GStreamer pipeline
    gst_element_set_state(gstPipeline, GST_STATE_PLAYING);

    // Start the GStreamer main loop in a new thread
    g_main_context_invoke(NULL, (GSourceFunc)startMainLoop, this);
}

void GstRgbdServer::cleanup() {
    // Stop the RealSense pipeline and release resources
    pipeline.stop();

    // Stop GStreamer pipeline
    gst_element_set_state(gstPipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(gstPipeline));

    // Cleanup GStreamer main loop
    g_main_loop_quit(mainLoop);
    g_main_loop_unref(mainLoop);
}

void GstRgbdServer::startStreaming() {
    // Do nothing, streaming is handled by the GStreamer main loop
}

void GstRgbdServer::stopStreaming() {
    // Stop the GStreamer main loop
    g_main_loop_quit(mainLoop);
}

GstFlowReturn GstRgbdServer::newSample(GstElement* appsrc, gpointer user_data) {
    // Callback function to push frames to appsrc
    GstRgbdServer* server = static_cast<GstRgbdServer*>(user_data);

    // Implement frame retrieval logic here
    rs2::frameset frames = server->pipeline.wait_for_frames();
    rs2::video_frame color_frame = frames.get_color_frame();
    const uint8_t* frame_data = reinterpret_cast<const uint8_t*>(color_frame.get_data());

    // Push the frame data to the GStreamer appsrc
    gst_app_src_push_buffer(GST_APP_SRC(appsrc),
                            gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,
                                                        const_cast<uint8_t*>(frame_data),
                                                        color_frame.get_height() * color_frame.get_stride_in_bytes(),
                                                        0,
                                                        color_frame.get_height(),
                                                        NULL,
                                                        NULL));

    return GST_FLOW_OK;
}

void GstRgbdServer::startMainLoop(GstRgbdServer* server) {
    // Start the GStreamer main loop
    server->mainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(server->mainLoop);
}