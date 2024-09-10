#pragma once 
#include <fstream> 
#include <iostream> 

/* in file file_checks.cpp 
 *
 * function checks if the uploaded file's MIME type 
 * is one of the supported file types 
 */
bool is_valid_mime_type(const std::string&);

/* in file file_checks.cpp 
 *
 * function checks the file signature 
 * of the uploaded file (first few bytes)
 * to verify it is supported and non-malicious
 */
bool is_valid_file_signature(const std::string&); 
