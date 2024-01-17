#include <iostream>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <librealsense2/rs.hpp>
#include <gst/app/gstappsrc.h>

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create a GStreamer pipeline
    GstElement *pipeline = gst_pipeline_new("realsense_pipeline");

    // Create elements
    GstElement *source = gst_element_factory_make("appsrc", "video-source");
    GstElement *convert = gst_element_factory_make("videoconvert", "video-convert");
    GstElement *encoder = gst_element_factory_make("nvv4l2h264enc", "h264-encoder");
    GstElement *payloader = gst_element_factory_make("rtph264pay", "h264-payloader");
    GstElement *sink = gst_element_factory_make("udpsink", "udp-sink");

    if (!pipeline || !source || !convert || !encoder || !payloader || !sink) {
        g_error("Failed to create elements.");
        return -1;
    }

    // Set properties for the appsrc element
    g_object_set(G_OBJECT(source),
             "is-live", TRUE,
             "num-buffers", -1,
             "block", TRUE,
             "caps", gst_caps_from_string("video/x-raw, format=RGB"),
             nullptr);
    // Set properties for the encoder
    g_object_set(G_OBJECT(encoder), "bitrate", 2000000, nullptr);

    // Set properties for the UDP sink (change the IP and port accordingly)
    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", "port", 5000, nullptr);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, convert, encoder, payloader, sink, nullptr);

    // Link elements
    if (!gst_element_link_many(source, convert, encoder, payloader, sink, nullptr)) {
        g_error("Failed to link elements.");
        return -1;
    }

    // Set the pipeline to the playing state
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // RealSense configuration
    rs2::config cfg;
    rs2::pipeline pipe;
    rs2::pipeline_profile profile;

    // Enable the RGB stream
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);

    // Start the pipeline
    pipe.start(cfg);

    // Main loop to capture and feed frames into GStreamer pipeline
    while (1) {
        rs2::frameset frames;
        if (pipe.poll_for_frames(&frames)) {
            // Get the RGB frame
            rs2::video_frame color_frame = frames.get_color_frame();

            // Send the frame to GStreamer appsrc
            GstBuffer *buffer = gst_buffer_new_and_alloc(color_frame.get_width() * color_frame.get_height() * 3);
            gst_buffer_fill(buffer, 0, color_frame.get_data(), color_frame.get_width() * color_frame.get_height() * 3);
            gst_app_src_push_buffer(GST_APP_SRC(source), buffer);
        }

        // Handle GStreamer messages if any
        GstBus *bus = gst_element_get_bus(pipeline);
        GstMessage *msg = gst_bus_pop(bus);

        if (msg != nullptr) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error(msg, &err, &debug_info);
                    g_error("Error: %s", err->message);
                    g_error_free(err);
                    g_free(debug_info);
                    break;

                case GST_MESSAGE_EOS:
                    g_print("End of stream.\n");
                    break;

                default:
                    // Ignore other messages
                    break;
            }

            gst_message_unref(msg);
        }

        gst_object_unref(bus);
    }

    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
