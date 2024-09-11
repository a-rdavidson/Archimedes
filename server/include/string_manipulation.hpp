#pragma once 

#include <iostream>
#include <vector>
#include <cstring> 

/* 
 * For use to split the flags passed to the server
 * from the client as a std::string into a char ** 
 * so they can be passed to rgb_matrix::ParseOptionsFromFlags
 * to begin initialization process of rgb matrix
 */ 
char ** string_to_char_array(const std::string&, int);

/*
 * only for use with char** returned from 
 * string_to_char_array() 
 */
 void free_char_array(char**, int);
