#define CROW_MAIN

#include "crow_all.h" 
#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"
#include "file_checks.hpp"
#include "string_manipulation.hpp"
#include "image_handler.hpp"

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
#include <thread>
#include <future>

using rgb_matrix::StreamReader;

static void SignalHandler(int signo) {
  interrupt_received = true;
}
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
  
  //signal(SIGTERM, SignalHandler);
  //signal(SIGINT,  SignalHandler);

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
  * Route to Clear Matrix 
  */
  CROW_ROUTE(app, "/clear") 
  ([]() {
   //clear_matrix(); 
   return crow::response(200, "Matrix Cleared"); 

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
  
  std::string file_data, flags, loopCount, frameDelay, displayTime;  
  crow::multipart::message_view msg(req); 

  file_data     = msg.parts[0].body;
  flags         = msg.parts[1].body; 
  loopCount     = msg.parts[2].body; 
  frameDelay    = msg.parts[3].body;
  displayTime   = msg.parts[4].body;

  std::string file_path = "/tmp/uploaded_file";
  if (!file_data.empty() ) {
      std::ofstream file(file_path, std::ios::binary); 
      file.write(file_data.c_str(), file_data.size()); 
      file.close();
      std::cout << "Saved file to disk" << std::endl;
    }
    
    if(!is_valid_file_signature(file_path)) {
      try {
        std::filesystem::remove(file_path); 
      } catch (const std::filesystem::filesystem_error& err) {
        std::cout << "Filesystem error: " << err.what() << std::endl; 
      }
      return crow::response(400, "Invalid file signature. File Format not supported");
    }

  for (int i = 1; i < msg.parts.size(); i++) {
    std::cout << "i: " << i << msg.parts[i] << std::endl;
  }

  struct UploadedData image_data;

  image_data.file_path        = file_path; 
  image_data.flags            = flags; 
  image_data.animFrameDelay   = frameDelay.empty()  ? 80 : std::stoi(frameDelay); 
  image_data.loopCount        = loopCount.empty()   ? 999999   : std::stoi(loopCount); 
  image_data.displayTime      = displayTime.empty() ? 999999 : std::stoi(displayTime); 
  
  image_handler(image_data); 

    return crow::response(200, "Image received and processing");
  });
  
  app.port(8080).multithreaded().run(); 
  
  return 0; 
}
