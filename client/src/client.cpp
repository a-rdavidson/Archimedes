#include <iostream>
#include <fstream>
#include <curl/curl.h>

int send_file(const std::string& file_path, const std::string& flags) {
    CURL* curl;
    CURLcode res;
    curl_mime* mime;
    curl_mimepart* part;
    curl = curl_easy_init();
    
    if (curl) {
        mime = curl_mime_init(curl);
        
        // Add file part 
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "file");
        curl_mime_filedata(part, file_path.c_str());
        
        // Add the flags part
        part = curl_mime_addpart(mime); 
        curl_mime_name(part, "flags");
        curl_mime_data(part, flags.c_str(), CURL_ZERO_TERMINATED); 

        curl_easy_setopt(curl, CURLOPT_URL, "http://10.7.48.39:8080/upload");

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "File uploaded successfully!" << std::endl;
        }

        curl_mime_free(mime);
        curl_easy_cleanup(curl);
    }
    
    return 0;
}

int main(int argc, char ** argv) {
    std::string file_path = argv[1];
    std::string flags;
    for (int i = 2; i < argc; i++) {
      flags = flags + argv[i] + " ";
    }
    std::cout << "flags: " << flags << std::endl; 
    std::cout << "file_path: " << file_path << std::endl; 

    send_file(file_path, flags);
    return 0;
}

