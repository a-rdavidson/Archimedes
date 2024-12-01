#pragma once

#include <cstdint>
#include <string>
#include "led-matrix.h"
#include <atomic>

using rgb_matrix::RGBMatrix;

/* Global Variables */
extern std::atomic<bool> interrupt_received;
extern RGBMatrix * matrix;
extern std::atomic<bool> stopThread;
extern std::atomic<int> threadCount;
typedef int64_t tmillis_t;

struct UploadedData {
  std::string file_path; 
  std::string flags; 
  int         animFrameDelay;   /* If animation delay between frames */
  int         loopCount;        /* If animation how many complete loops */
  int         displayTime; 
  
};

void image_handler(struct UploadedData);

void clear_matrix(void);
