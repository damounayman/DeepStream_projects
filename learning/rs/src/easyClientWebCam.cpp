#include <gst/gst.h>

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    // Create a GStreamer pipeline
    GstElement* pipeline = gst_parse_launch("udpsrc port=5000 ! application/x-rtp,media=video,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false", NULL);

    if (!pipeline) {
        g_print("Failed to create GStreamer pipeline.\n");
        return 1;
    }

    // Set the pipeline state to playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Main loop
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_poll(bus, GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE);

    if (msg) {
        gst_message_unref(msg);
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}