#include <gst/gst.h>
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

const int WIDTH = 640;
const int HEIGHT = 480;
const int FRAMERATE = 30;

void setSourceCaps(GstElement *source, const char *format, int width, int height, int framerate) {
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, format,
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                                        NULL);
    g_object_set(G_OBJECT(source), "caps", caps, NULL);
    gst_caps_unref(caps);
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    GstElement *color_source = gst_element_factory_make("appsrc", "color-source");
    GstElement *depth_source = gst_element_factory_make("appsrc", "depth-source");
    GstElement *color_convert = gst_element_factory_make("videoconvert", "color-convert");
    GstElement *depth_convert = gst_element_factory_make("videoconvert", "depth-convert");
    GstElement *color_encoder = gst_element_factory_make("x264enc", "color-encoder");
    GstElement *depth_encoder = gst_element_factory_make("x264enc", "depth-encoder");
    GstElement *color_payloader = gst_element_factory_make("rtph264pay", "color-payloader");
    GstElement *depth_payloader = gst_element_factory_make("rtph264pay", "depth-payloader");
    GstElement *color_udpsink = gst_element_factory_make("udpsink", "color-udpsink");
    GstElement *depth_udpsink = gst_element_factory_make("udpsink", "depth-udpsink");

    if (!color_source || !depth_source || !color_convert || !depth_convert ||
        !color_encoder || !depth_encoder || !color_payloader || !depth_payloader ||
        !color_udpsink || !depth_udpsink) {
        g_printerr("Error: Could not create GStreamer elements.\n");
        return -1;
    }

    GstElement *pipeline = gst_pipeline_new("realsense-pipeline");
    if (!pipeline) {
        g_printerr("Error: Could not create GStreamer pipeline.\n");
        return -1;
    }

    g_object_set(G_OBJECT(color_source), "name", "color-source", NULL);
    setSourceCaps(color_source, "BGR", WIDTH, HEIGHT, FRAMERATE);

    g_object_set(G_OBJECT(depth_source), "name", "depth-source", NULL);
    setSourceCaps(depth_source, "GRAY16_LE", WIDTH, HEIGHT, FRAMERATE);

    g_object_set(G_OBJECT(color_udpsink), "auto-multicast", true, "force-ipv4",
                 true, "host", "127.0.0.1", "port", 5000, "sync", false, NULL);
    g_object_set(G_OBJECT(depth_udpsink), "auto-multicast", true, "force-ipv4",
                 true, "host", "127.0.0.1", "port", 5001, "sync", false, NULL);

    gst_bin_add_many(GST_BIN(pipeline), color_source, color_convert, color_encoder,
                     color_payloader, color_udpsink, depth_source, depth_convert,
                     depth_encoder, depth_payloader, depth_udpsink, NULL);

    if (!gst_element_link_many(color_source, color_convert, color_encoder,
                               color_payloader, color_udpsink, NULL) ||
        !gst_element_link_many(depth_source, depth_convert, depth_encoder,
                               depth_payloader, depth_udpsink, NULL)) {
        g_printerr("Error: Could not link GStreamer elements.\n");
        return -1;
    }

    GstStateChangeReturn state_ret =
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (state_ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    rs2::pipeline pipeline_rs2;
    rs2::config config;
    config.enable_stream(RS2_STREAM_COLOR, WIDTH, HEIGHT, RS2_FORMAT_BGR8, FRAMERATE);
    config.enable_stream(RS2_STREAM_DEPTH, WIDTH, HEIGHT, RS2_FORMAT_Z16, FRAMERATE);
    pipeline_rs2.start(config);

    cv::Mat color_frame, depth_frame;
    while (true) {
        rs2::frameset frames = pipeline_rs2.wait_for_frames();
        auto color_frame_rs2 = frames.get_color_frame();
        auto depth_frame_rs2 = frames.get_depth_frame();

        color_frame = cv::Mat(cv::Size(WIDTH, HEIGHT), CV_8UC3,
                              (void *)color_frame_rs2.get_data());
        auto unscaled = cv::Mat(cv::Size(WIDTH, HEIGHT), CV_16UC1,
                                (void *)depth_frame_rs2.get_data());
        auto units = depth_frame_rs2.get_units();
        unscaled.convertTo(depth_frame, CV_64F, units);

        GstBuffer *color_buffer = gst_buffer_new_wrapped(
            color_frame.data, color_frame.total() * color_frame.elemSize());
        GstBuffer *depth_buffer = gst_buffer_new_wrapped(
            unscaled.data, unscaled.total() * unscaled.elemSize());

        GstFlowReturn ret;
        g_signal_emit_by_name(color_source, "push-buffer", color_buffer, &ret);
        if (ret != GST_FLOW_OK) {
            g_printerr("Error: Failed to push buffer to color pipeline.\n");
            break;
        }

        g_signal_emit_by_name(depth_source, "push-buffer", depth_buffer, &ret);
        if (ret != GST_FLOW_OK) {
            g_printerr("Error: Failed to push buffer to depth pipeline.\n");
            break;
        }

        cv::imshow("Color Pub", color_frame);
        cv::imshow("Depth Pub", unscaled);

        if (cv::waitKey(30) == 27) {
            break;
        }
    }

    pipeline_rs2.stop();
    cv::destroyAllWindows();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_deinit();

    return 0;
}