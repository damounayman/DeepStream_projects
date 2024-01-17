#include <iostream>
#include "gst_rgbd_server/gst_rgbd_server.h"


int main() {
    // Displaying a message to the console
    GstRgbdServer server;
    server.startStreaming();
    std::cout << "Hello, World!" << std::endl;

    // Returning 0 indicates successful completion of the program
    return 0;
}
