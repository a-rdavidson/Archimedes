#include "file_checks.hpp"


bool is_valid_mime_type(const std::string& mime_type) {
  std::vector<std::string> allowed_types = {
    "image/jpeg",
    "image/png",
    "image/gif",
    "video/mp4"
  };
  
  return std::find(allowed_types.begin(), allowed_types.end(), mime_type) != allowed_types.end(); 
}


bool is_valid_file_signature(const std::string& file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) return false;

  unsigned char buffer[8];
  file.read(reinterpret_cast<char*>(buffer), sizeof(buffer));

  // Check the magic numbers for different file types
  if (buffer[0] == 0xFF && buffer[1] == 0xD8) {
    // JPEG
    return true;
  } else if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E && buffer[3] == 0x47) {
    // PNG
    return true;
  } else if (buffer[0] == 0x47 && buffer[1] == 0x49 && buffer[2] == 0x46) {
    // GIF
    return true;
  } else if (buffer[4] == 0x66 && buffer[5] == 0x74 && buffer[6] == 0x79 && buffer[7] == 0x70) {
    // MP4
    return true;
  }
  return false; 
}
