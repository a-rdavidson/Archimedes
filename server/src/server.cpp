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

std::string extract_boundary(const std::string& content_type) {
    // Find the position of the "boundary=" part in the Content-Type header
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        // If "boundary=" is not found, return an empty string (invalid case)
        return "";
    }
    
    // The boundary string starts right after "boundary=" (which is 9 characters long)
    boundary_pos += 9;
    
    // Extract the boundary string from the Content-Type header
    std::string boundary = content_type.substr(boundary_pos);
    
    // Return the extracted boundary
    return boundary;
}

void parse_multipart_data(const crow::request& req, const std::string& body, std::string * file_data, std::string * flags) {
    auto content_type_it = req.headers.find("Content-Type"); 
    if (content_type_it == req.headers.end()) {
      return;
    }

    std::string content_type = content_type_it->second;
    std::string boundary = extract_boundary(content_type); 

    size_t boundary_pos = body.find(boundary);
    size_t file_pos = body.find("name=\"file\"", boundary_pos);
    size_t flags_pos = body.find("name=\"flags\"", file_pos);
    std::cout << "boundary_pos " << boundary_pos << " flags_pos " << flags_pos << " file_pos " << file_pos << std::endl;
    if (flags_pos != std::string::npos) {
        size_t flags_start = body.find("\r\n\r\n", flags_pos) + 4;
        size_t flags_end = body.find("\r\n", flags_start);
        std::cout << body.substr(flags_start, flags_end - flags_start) << std::endl;
        *flags = body.substr(flags_start, flags_end - flags_start);
    }

    if (file_pos != std::string::npos) {
        size_t file_start = body.find("\r\n\r\n", file_pos) + 4;
        size_t file_end = body.find(boundary, file_start) - 4;  // Subtract 4 for the final \r\n
        *file_data = body.substr(file_start, file_end - file_start);
    }
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
   clear_matrix(); 
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
    

    std::string file_data, flags;
    parse_multipart_data(req, req.body, &file_data, &flags); 
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


    std::cout << "Received flags: " << flags << std::endl;

    /* Counting number of flags */
    int numflags = countWords(flags); 
    
    auto future = std::async(std::launch::async, image_handler, flags, numflags);
     

    return crow::response(200, "Image received and processing");
  });
  
  std::thread server_thread([&app]() {
  app.port(8080).multithreaded().run(); 
  });
  
  // wait for interrupt_signals in main thread
  while (!interrupt_received) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
  }

  //stop server when an interrupt is received
    app.stop();
  if (server_thread.joinable() ) {
    server_thread.join();
  } 

  std::cout << "Server stopped gracefully." << std::endl;
  return 0; 
}
