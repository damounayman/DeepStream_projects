#include <iostream>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

// Callback function to handle new clients connected to the RTSP server
static void on_new_rtsp_client(GstRTSPServer *server, GstRTSPClient *client, gpointer user_data) {
    g_print("New RTSP client connected!\n");
}

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Check command-line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <video-source> <rtsp-address>" << std::endl;
        return 1;
    }

    // Create a GStreamer pipeline
    GstElement *pipeline = gst_pipeline_new("my_pipeline");

    // Create elements
    GstElement *source = gst_element_factory_make("v4l2src", "video-source");
    GstElement *convert = gst_element_factory_make("videoconvert", "video-convert");
    GstElement *sink = gst_element_factory_make("autovideosink", "video-sink");

    if (!pipeline || !source || !convert || !sink) {
        std::cerr << "Failed to create elements." << std::endl;
        return 1;
    }

    // Set properties for the video source (webcam)
    g_object_set(G_OBJECT(source), "device", argv[1], NULL);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, NULL);

    // Link elements
    if (!gst_element_link(source, convert) || !gst_element_link(convert, sink)) {
        std::cerr << "Failed to link elements." << std::endl;
        return 1;
    }

    // Create an RTSP server
    GstRTSPServer *server = gst_rtsp_server_new();
    g_object_set(G_OBJECT(server), "service", argv[2], NULL);

    // Create an RTSP media factory
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, "( videotestsrc ! videoconvert ! autovideosink )");
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    // Attach the RTSP media factory to the server
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    // Set up the signal handler for new clients
    g_signal_connect(server, "client-connected", G_CALLBACK(on_new_rtsp_client), NULL);

    // Attach the RTSP server to the pipeline
    gst_rtsp_server_attach(server, NULL);

    // Set the pipeline to the playing state
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS (End Of Stream)
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Parse message
    if (msg != nullptr) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                std::cerr << "Error: " << err->message << std::endl;
                g_error_free(err);
                g_free(debug_info);
                break;

            case GST_MESSAGE_EOS:
                std::cout << "End of stream." << std::endl;
                break;

            default:
                // Ignore other messages
                break;
        }

        gst_message_unref(msg);
    }

    // Free resources
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
