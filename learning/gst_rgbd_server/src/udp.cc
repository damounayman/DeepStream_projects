#include <librealsense2/rs.hpp>
#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Initialize RealSense pipeline
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);

    // Start RealSense pipeline
    pipe.start(cfg);

    // Create GStreamer pipeline
    GstElement *pipeline = gst_pipeline_new("realsense-pipeline");

    // Create elements
    GstElement *source = gst_element_factory_make("appsrc", "source");
    GstElement *convert = gst_element_factory_make("nvvideoconvert", "convert");
    GstElement *encoder = gst_element_factory_make("nvv4l2h264enc", "encoder"); // NVIDIA H.264 encoder
    GstElement *payloader = gst_element_factory_make("rtph264pay", "payloader"); // RTSP payloader
    GstElement *udpsink = gst_element_factory_make("udpsink", "udpsink");

    // Check if elements are created successfully
    if (!pipeline || !source || !convert || !encoder || !payloader || !udpsink) {
        std::cerr << "Error creating elements!" << std::endl;
        return -1;
    }

    // Set properties for the appsrc element
    g_object_set(G_OBJECT(source), "is-live", TRUE, "caps",
                 gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", NULL), NULL);

    // Set properties for udpsink (change the IP and port as needed)
    g_object_set(G_OBJECT(udpsink), "host", "127.0.0.1", "port", 5000, NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, convert, encoder, payloader, udpsink, NULL);
    if (!gst_element_link_many(source, convert, encoder, payloader, udpsink, NULL)) {
        std::cerr << "Error linking elements!" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Set the pipeline state to playing
    GstStateChangeReturn state_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (state_ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Main loop
    while (true) {
        // Wait for the next set of frames from the RealSense camera
        rs2::frameset frames = pipe.wait_for_frames();

        // Get the color frame
        rs2::video_frame color_frame = frames.get_color_frame();

        // Create GStreamer buffer and push it to the pipeline
        GstBuffer *buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                                                const_cast<void*>(color_frame.get_data()),
                                                                color_frame.get_height() * color_frame.get_stride_in_bytes(),
                                                                0,
                                                                color_frame.get_height() * color_frame.get_stride_in_bytes(),
                                                                nullptr,
                                                                nullptr));

        GstFlowReturn ret;
        g_signal_emit_by_name(source, "push-buffer", buffer, &ret);

        gst_buffer_unref(buffer);
    }

    // Cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    pipe.stop();

    return 0;
}
