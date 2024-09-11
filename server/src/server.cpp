#define CROW_MAIN

#include "crow_all.h" 
#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"
#include "file_checks.hpp"

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
#include <filesystem>

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
    std::string file_data = req.body; 
    std::string content_type = req.get_header_value("Content-Type"); 

    if(!is_valid_mime_type(content_type)) {
      
      return crow::response(400, "Invalid File Type. Only JPG, PNG, GIF, & MP4"); 
    }

    std::string file_path = "/tmp/uploaded_file"; 
    std::ofstream file(file_path, std::ios::binary); 
    file.write(req.body.c_str(), req.body.size()); 
    file.close();

    if(!is_valid_file_signature(file_path)) {
      try {
        std::filesystem::remove(file_path); 
      } catch (const std::filesystem::filesystem_error& err) {
        std::cout << "Filesystem error: " << err.what() << std::endl; 
      }
      return crow::response(400, "Invalid file signature. File Format not supported");
    }


    std::string flags = req.url_params.get("flags") ? req.url_params.get("flags") : "none"; 
    std::cout << "Received flags: " << flags << std::endl;

    char delimeter = ' '; 
    int delim_count = 0; 

    for (int i = 0; i < flags.size(); i++) {
      if (flags[i] == delimeter) {
        count++;
      }
    }
    
    char ** argv = string_to_char_array(flags, delim_count + 1);
    
    for (int i = 0; i < delim_count + 1; i++) { 
      std::cout << "argv[" << i << "] " << argv[i]; 
    }
    std::cout << "delim_count: " << delim_count;

    RGBMatrix::Options matrix_options; 
    rgb_matrix::RuntimeOptions runtime_opt; 

    runtime_opt.drop_priv_user = getenv("SUDO_UID"); 
    runtime_opt.drop_priv_group = getenv("SUDO_GID"); 



    return crow::response(200, "Image received and processed");
  });
  app.port(8080).multithreaded().run(); 
  return 0; 
}
