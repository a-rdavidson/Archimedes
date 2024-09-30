#define CROW_MAIN

#include "crow_all.h" 
#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"
#include "file_checks.hpp"
#include "string_manipulation.hpp"
#include "multipart_parser.h"

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

// Data structure to hold parsed form data
struct FormData {
  std::string file_data; 
  std::string file_name;
  std::string flags;
};

/* 
 * File part callback
 */

int on_part_data(multipart_parser* parser, const char *at, size_t length, void * user_data) {
  FormData* form_data = static_cast<FormData*>(user_data);
  
  form_data->file_data.append(at, length);
  return 0;
}

int on_header_field(multipart_parser* parser, const char * at, size_t length, void * user_data) {
  FormData* form_data = static_cast<FormData*>(user_data);
  std::string header_field(at, length); 
  
  
  if (header_field.find("filename") != std::string::npos) {
    size_t start = header_field.find("filename=\"")  + 10; // 10 = length of "filename\"'
    size_t end = header_field.find("\"");
    form_data->file_name = header_field.substr(start, end - start);
  }
  if (header_field.find("name=\"flags\"") != std::string::npos) {
    form_data->flags = header_field.substr(length);
  }
  return 0;
}

int on_part_data_end(multipart_parser* parser, void * user_data) {
  return 0; 
}

std::string extract_boundary(const std::string& content_type) {
  size_t boundary_pos = content_type.find("boundary=");
  if (boundary_pos == std::string::npos) {
    return "";
  }

  boundary_pos += 9;    /* Boundary String starts after "boundary=" */
                        /* which is 9 chars long */  
  std::string boundary = content_type.substr(boundary_pos); 

  return boundary;
}

/* 
 * MORE MULTIPART FORM HANDLING CALLBACKS GO HERE
 * Unfinished 
 */

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
      
    auto content_type_it = req.headers.find("Content-Type");

    if (content_type_it == req.headers.end()) {
      return crow::response(400, "Missing Content-type header");
    }

    std::string content_type = content_type_it->second;
    std::string boundary = extract_boundary(content_type);
    
    if(boundary.empty() ){
      return crow::response(400, "Invalid Content-Type: boundary not found");
    }
    
    // Debugging printout
    std::cout << "Boundary: " << boundary << std::endl;

   multipart_parser parser; 
   FormData form_data; 
   multipart_parser_init(&parser, boundary.c_str()); 
   parser.data = &form_data;
   
   // setting callback function ptr for parser
   multipart_parser_callbacks callbacks;
   callbacks.on_part_data = on_part_data; 
   callbacks.on_header_field = on_header_field; 
   callbacks.on_part_data_end = on_part_data_end;

   size_t parsed_bytes = multipart_parser_execute(&parser, req.body.c_str(), req.body.size());

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

    /* Counting number of flags */
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
