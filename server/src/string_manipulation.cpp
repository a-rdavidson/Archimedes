#include "string_manipulation.hpp" 

char** string_to_char_array(const std::string& input, int& num_words) {
  std::vector<std::string> words; 
  std::string word; 
  for (size_t i = 0; i < input.size(); i++) {
    if (input[i] == ' ' ) {
      if (!word.empty()) {
        words.push_back(word); 
        word.clear(); 
      }
    } else {
      word += input[i];
    }
  }
  if (!word.empty()) {
    words.push_back(word); 
  }

  num_words = words.size(); 

  char ** result = new char*[num_words];

  for (int i = 0; i < num_words; i++) {
    result[i] = new char[words[i].size() + 1]; 
    std::strcpy(result[i], words[i].c_str());
  }
  return result;
}

void free_char_array(char** arr, int num_words) { 
  for (int i = 0; i < num_words; i++) {
    delete[] arr[i];
  }
  delete[] arr;
}
