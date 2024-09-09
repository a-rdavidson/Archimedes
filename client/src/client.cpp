#include <iostream>
#include <fstream>
#include <curl/curl.h>

int upload_file(const std::string& file_path) {
    CURL* curl;
    CURLcode res;
    curl_mime* mime;
    curl_mimepart* part;
    curl = curl_easy_init();
    
    if (curl) {
        mime = curl_mime_init(curl);
        
        // Add file part to the request
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "file");
        curl_mime_filedata(part, file_path.c_str());
        
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
    std::cout << "file_path: " << file_path << std::endl; 

    upload_file(file_path);
    return 0;
}

