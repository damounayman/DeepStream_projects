#include <gst/gst.h>
#include <opencv2/opencv.hpp>



int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);



    // Create a GStreamer pipeline to receive RGB video via UDP
    GstElement *rgb_pipeline = gst_parse_launch("udpsrc port=5000 ! application/x-rtp,media=video,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink_rgb sync=false", NULL);

    // Create a GStreamer pipeline to receive depth frames via UDP
    GstElement *depth_pipeline = gst_parse_launch("udpsrc port=5001 ! application/x-rtp,media=video,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=GRAY16_LE ! appsink name=appsink_depth sync=false", NULL);

    if (!rgb_pipeline || !depth_pipeline) {
        g_print("Failed to create GStreamer pipelines.\n");
        return 1;
    }

    // Set the pipelines' state to playing
    gst_element_set_state(rgb_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(depth_pipeline, GST_STATE_PLAYING);

    cv::namedWindow("RGB Video", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Depth Video", cv::WINDOW_AUTOSIZE);

    // Main loop
    GstBus *rgb_bus = gst_element_get_bus(rgb_pipeline);
    GstBus *depth_bus = gst_element_get_bus(depth_pipeline);
    GstMessage *rgb_msg, *depth_msg;
    cv::Mat rgb_frame, depth_frame;

    while (true) {
        // Retrieve frames from RGB pipeline
        GstSample *rgb_sample = nullptr;
        g_signal_emit_by_name(gst_bin_get_by_name(GST_BIN(rgb_pipeline), "appsink_rgb"), "pull-sample", &rgb_sample);
        if (rgb_sample) {
            GstBuffer *rgb_buffer = gst_sample_get_buffer(rgb_sample);
            GstMapInfo rgb_info;
            if (gst_buffer_map(rgb_buffer, &rgb_info, GST_MAP_READ)) {
                // Process the RGB frame (rgb_info.data) here
                rgb_frame = cv::Mat(cv::Size(640, 480), CV_8UC3, rgb_info.data);
                gst_buffer_unmap(rgb_buffer, &rgb_info);
                gst_sample_unref(rgb_sample);

                if (!rgb_frame.empty()) {
                    cv::imshow("RGB Video", rgb_frame);
                }
            }
        }

        // Retrieve frames from depth pipeline
        GstSample *depth_sample = nullptr;
        g_signal_emit_by_name(gst_bin_get_by_name(GST_BIN(depth_pipeline), "appsink_depth"), "pull-sample", &depth_sample);
        if (depth_sample) {
            GstBuffer *depth_buffer = gst_sample_get_buffer(depth_sample);
            GstMapInfo depth_info;
            if (gst_buffer_map(depth_buffer, &depth_info, GST_MAP_READ)) {
                auto unscaled = cv::Mat(cv::Size(640, 480), CV_16UC1, (void*)depth_info.data);
                unscaled.convertTo(depth_frame, CV_64F, 0.001);
                if (!depth_frame.empty()) {
                    cv::imshow("Depth Video", unscaled);
                }
                gst_buffer_unmap(depth_buffer, &depth_info);
                gst_sample_unref(depth_sample);
            }
        }

        if (cv::waitKey(30) == 27) {  // Exit if the Esc key is pressed
            break;
        }
    }

    // Release resources
    cv::destroyAllWindows();
    gst_element_set_state(rgb_pipeline, GST_STATE_NULL);
    gst_element_set_state(depth_pipeline, GST_STATE_NULL);
    gst_object_unref(rgb_pipeline);
    gst_object_unref(depth_pipeline);
    gst_deinit();

    return 0;
}

// #include <iostream>
// #include <sstream>

// #include <gst/gst.h>
// #include <glib.h>

// static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
//     GMainLoop *loop = (GMainLoop *)data;

//     switch (GST_MESSAGE_TYPE(msg)) {
//         case GST_MESSAGE_EOS:
//             g_print("End of stream\n");
//             g_main_loop_quit(loop);
//             break;

//         case GST_MESSAGE_ERROR: {
//             gchar *debug;
//             GError *error;

//             gst_message_parse_error(msg, &error, &debug);
//             g_free(debug);

//             g_printerr("Error: %s\n", error->message);
//             g_error_free(error);

//             g_main_loop_quit(loop);
//             break;
//         }
//         default:
//             break;
//     }

//     return TRUE;
// }

// static int runGstreamer(int port, const char *pipelineName) {
//     GMainLoop *loop;

//     GstElement *pipeline, *source, *demux, *conv, *sink, *rtpdec, *h264dec, *h264parse;
//     GstBus *bus;
//     guint bus_watch_id;

//     loop = g_main_loop_new(NULL, FALSE);

//     /* Create gstreamer elements */
//     pipeline = gst_pipeline_new(pipelineName);
//     source = gst_element_factory_make("udpsrc", "udp-input");
//     conv = gst_element_factory_make("videoconvert", "converter");
//     rtpdec = gst_element_factory_make("rtph264depay", "rtp-decode");
//     h264dec = gst_element_factory_make("avdec_h264", "h264-decode");
//     h264parse = gst_element_factory_make("h264parse", "h264-parse");
//     sink = gst_element_factory_make("autovideosink", "video-output");

//     if (!pipeline || !source || !conv || !sink || !rtpdec || !h264dec || !h264parse) {
//         g_printerr("One element could not be created. Exiting.\n");
//         return -1;
//     }

//     /* Set up the pipeline */
//     /* we set the input filename to the source element */
//     g_object_set(G_OBJECT(source), "port", port, NULL);

//     /* we add a message handler */
//     bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
//     bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
//     gst_object_unref(bus);

//     /* we add all elements into the pipeline */
//     gst_bin_add_many(GST_BIN(pipeline), source, rtpdec, h264dec, conv, sink, h264parse, NULL);

//     gboolean link_ok;
//     GstCaps *caps;

//     caps = gst_caps_new_simple("application/x-rtp",
//         "encoding-name", G_TYPE_STRING, "H264",
//         "payload", G_TYPE_INT, 96,
//         "clock-rate", G_TYPE_INT, 90000,
//         "media", G_TYPE_STRING, "video",
//         NULL);

//     link_ok = gst_element_link_filtered(source, rtpdec, caps);
//     gst_caps_unref(caps);

//     if (!link_ok) {
//         g_warning("Failed to link rtpenc and sink!");
//     }

//     gboolean link_many_ok;
//     /* we link the elements together */
//     link_many_ok = gst_element_link_many(rtpdec, h264parse, h264dec, conv, sink, NULL);

//     if (!link_many_ok) {
//         g_warning("Failed to link remaining elements!");
//     }

//     /* Set the pipeline to "playing" state*/
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     /* Iterate */
//     g_print("Running...\n");
//     g_main_loop_run(loop);

//     /* Out of the main loop, clean up nicely */
//     g_print("Returned, stopping playback\n");
//     gst_element_set_state(pipeline, GST_STATE_NULL);

//     g_print("Deleting pipeline\n");
//     gst_object_unref(GST_OBJECT(pipeline));
//     g_source_remove(bus_watch_id);
//     g_main_loop_unref(loop);

//     return 0;
// }

// int main(int argc, char *argv[]) {
//     int rgbPort = 5000;
//     int depthPort = 5001;

//     for (int i = 0; i < argc; i++) {
//         if (strcmp(argv[i], "-r") == 0) {
//             std::istringstream ss(argv[i + 1]);
//             if (!(ss >> rgbPort)) {
//                 std::cerr << "Invalid RGB port: " << argv[i + 1] << '\n';
//             } else if (!ss.eof()) {
//                 std::cerr << "Trailing characters after RGB port: " << argv[i + 1] << '\n';
//             }
//         }
//         else if (strcmp(argv[i], "-d") == 0) {
//             std::istringstream ss(argv[i + 1]);
//             if (!(ss >> depthPort)) {
//                 std::cerr << "Invalid depth port: " << argv[i + 1] << '\n';
//             } else if (!ss.eof()) {
//                 std::cerr << "Trailing characters after depth port: " << argv[i + 1] << '\n';
//             }
//         }
//     }

//     std::cout << "RGB Port set to: " << rgbPort << std::endl;
//     std::cout << "Depth Port set to: " << depthPort << std::endl;

//     /* Gstreamer initialization */
//     gst_init(&argc, &argv);

//     // Run GStreamer for RGB
//     runGstreamer(rgbPort, "rgb_pipeline");

//     // Run GStreamer for Depth
//     runGstreamer(depthPort, "depth_pipeline");

//     return 0;
// }
