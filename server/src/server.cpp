#define CROW_MAIN

#include "crow_all.h" 
#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"

#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <Magick++.h>
#include <magick/image.h>

using rgb_matrix::Canvas;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamReader;

int main() {
  crow::SimpleApp app; 

  CROW_ROUTE(app, "/upload").methods(crow::HTTPMethod::Post)
  ([](const crow::request& req) {
    std::ofstream file("/tmp/uploaded_file", std::ios::binary); 
    file.write(req.body.c_str(), req.body.size()); 
    file.close(); 
    

    RGBMatrix::Options matrix_options; 
    rgb_matrix::RuntimeOptions runtime_opt; 

    runtime_opt.drop_priv_user = getenv("SUDO_UID"); 
    runtime_opt.drop_priv_group = getenv("SUDO_GID"); 



    return crow::response(200, "Image received and processed");
  });
  app.port(8080).multithreaded().run(); 
  return 0; 
}
