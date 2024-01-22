#include <iostream>
#include "gst_rgbd_server/gst_rgbd_server.h"


int main() {
    // Displaying a message to the console
    GstRgbdServer server;
    std::cout << "Hello, World!" << std::endl;
    server.stream();
    std::cout << "Hello, World!" << std::endl;

    server.update();

    // Returning 0 indicates successful completion of the program
    return 0;
}
