#define CROW_MAIN

#include "crow_all.h" 
#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"
#include "file_checks.hpp"
#include "string_manipulation.hpp"

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

std::string read_file(const std::string& file_path) {
  std::ifstream file(file_path); 
  if (! file.is_open()) {
    return ""; // empty return if file cannot be openned
  }

  std::stringstream buffer;
  buffer << file.rdbuf();     // read whole file into buffer
  return buffer.str();
}


int main() {
  crow::SimpleApp app; 

  /* 
   * Route to serve HTML page
   */ 
  
  CROW_ROUTE(app, "/")
  ([]() {
    std::string html_content = read_file("../html/upload.html");
    if(html_content.empty()) {
      return crow::response(404, "HTML file not found");
    }
    return crow::response(html_content);

  });
  
 /* 
   * Route to handle favicon.ico request
   */ 

  CROW_ROUTE(app, "/favicon.ico")
  ([]() {
    return crow::response(200, "");  // Respond with an empty body to prevent the 404 error
  });
  
  /* 
   * Route to handle file & flags upload
   */ 
  
  CROW_ROUTE(app, "/upload").methods(crow::HTTPMethod::Post)
  ([](const crow::request& req) {
    
    std::string file_data = req.body;
    std::string file_path = "/tmp/uploaded_file"; 
    
    std::ofstream file(file_path, std::ios::binary); 
    file.write(req.body.c_str(), req.body.size()); 
    file.close();

    if(!is_valid_file_signature(file_path)) {
      try {
        //std::filesystem::remove(file_path); 
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
        delim_count++;
      }
    }
    
    int num_flags = delim_count + 1;
    char ** client_argv; string_to_char_array(flags, num_flags);
    
    for (int i = 0; i < num_flags; i++) {
      std:: cout << client_argv[i] <<  std::endl;
    }

    RGBMatrix::Options matrix_options; 
    rgb_matrix::RuntimeOptions runtime_opt; 

    runtime_opt.drop_priv_user = getenv("SUDO_UID"); 
    runtime_opt.drop_priv_group = getenv("SUDO_GID"); 

    free_char_array(client_argv, num_flags); 

    return crow::response(200, "Image received and processed");
  });

  app.port(8080).multithreaded().run(); 
  return 0; 
}
