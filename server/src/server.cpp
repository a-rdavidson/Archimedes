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
#include <atomic> 


using rgb_matrix::StreamReader;

std::atomic<int> threadCount = 0; 
std::atomic<bool> stopThread(false);
std::atomic<bool> interrupt_received(false);

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
  
  signal(SIGTERM, SignalHandler);
  signal(SIGINT,  SignalHandler);

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
   
    stopThread = true;
        return crow::response(400, R"(
                <html>
                <head><title>Matrix Cleared</title></head>
                <body>
                    <script>
                        alert("Matrix Cleared");
                        window.location.href = "/";
                    </script>
                </body>
                </html>
            )");
 
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
  /* reset stopThread in to reuse thread for next use */ 
  
  std::cout << "threadCount " << threadCount << std::endl;

  if (threadCount > 0) {
        return crow::response(400, R"(
                <html>
                <head><title>Error</title></head>
                <body>
                    <script>
                        alert("An error occurred: the LED Matrix May Already be showing an image.");
                        window.location.href = "/";
                    </script>
                </body>
                </html>
            )");
  }

  stopThread = false;
  
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
      return crow::response(400, R"(
                <html>
                <head><title>Error</title></head>
                <body>
                    <script>
                        alert("An error occurred: The Filetype you Uploaded is Not Supported.");
                        window.location.href = "/";
                    </script>
                </body>
                </html>
            )");
    }
  
  for (size_t i = 1; i < msg.parts.size(); i++) {
    std::cout << "i: " << i << msg.parts[i] << std::endl;
  }

  struct UploadedData image_data;

  image_data.file_path        = file_path; 
  image_data.flags            = flags; 
  image_data.animFrameDelay   = frameDelay.empty()  ? 80     : std::stoi(frameDelay); 
  image_data.loopCount        = loopCount.empty()   ? 999999 : std::stoi(loopCount); 
  image_data.displayTime      = displayTime.empty() ? 999999 : std::stoi(displayTime); 
  

  std::cout << "animDelay " << image_data.animFrameDelay << std::endl;
  std::cout << "loopCount " << image_data.loopCount << std::endl;
  std::cout << "displayTime " << image_data.displayTime << std::endl;
  
  threadCount++;
  std::thread t1(image_handler, image_data); 
  t1.detach();   
  
  return crow::response(400, R"(
                <html>
                <head><title>Success</title></head>
                <body>
                    <script>
                        alert("Your File has been received and processed");
                        window.location.href = "/";
                    </script>
                </body>
                </html>
            )");
  });
  
  app.port(8080).multithreaded().run(); 
  
  return 0; 
}
