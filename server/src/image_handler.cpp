#include "image_handler.hpp"
#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"
#include "string_manipulation.hpp"

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
#include <iostream>

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
RGBMatrix * matrix;



typedef int64_t tmillis_t; 
static const tmillis_t distant_future = (1LL<<40); // very long time

struct ImageParams {
  ImageParams() : anim_duration_ms(distant_future), wait_ms(1500),
                  anim_delay_ms(-1), loops(-1), vsync_multiple(1) {}
  tmillis_t anim_duration_ms;  // If this is an animation, duration to show.
  tmillis_t wait_ms;           // Regular image: duration to show.
  tmillis_t anim_delay_ms;     // Animation delay override.
  int loops;
  int vsync_multiple;
};

struct FileInfo {
  ImageParams params;      // Each file might have specific timing settings
  bool is_multi_frame = false;
  rgb_matrix::StreamIO *content_stream = nullptr;
};


static tmillis_t GetTimeInMillis() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void SleepMillis(tmillis_t milli_seconds) {
  if (milli_seconds <= 0) return;
  struct timespec ts;
  ts.tv_sec = milli_seconds / 1000;
  ts.tv_nsec = (milli_seconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

static void StoreInStream(const Magick::Image &img, int delay_time_us,
                          bool do_center,
                          rgb_matrix::FrameCanvas *scratch,
                          rgb_matrix::StreamWriter *output) {
  scratch->Clear();
  const int x_offset = do_center ? (scratch->width() - img.columns()) / 2 : 0;
  const int y_offset = do_center ? (scratch->height() - img.rows()) / 2 : 0;
  for (size_t y = 0; y < img.rows(); ++y) {
    for (size_t x = 0; x < img.columns(); ++x) {
      const Magick::Color &c = img.pixelColor(x, y);
      if (c.alphaQuantum() < 255) {
        scratch->SetPixel(x + x_offset, y + y_offset,
                          ScaleQuantumToChar(c.redQuantum()),
                          ScaleQuantumToChar(c.greenQuantum()),
                          ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
  output->Stream(*scratch, delay_time_us);
}

static void CopyStream(rgb_matrix::StreamReader *r,
                       rgb_matrix::StreamWriter *w,
                       rgb_matrix::FrameCanvas *scratch) {
  uint32_t delay_us;
  while (r->GetNext(scratch, &delay_us)) {
    w->Stream(*scratch, delay_us);
  }
}

// Load still image or animation.
// Scale, so that it fits in "width" and "height" and store in "result".
static bool LoadImageAndScale(const char *filename,
                              int target_width, int target_height,
                              bool fill_width, bool fill_height,
                              std::vector<Magick::Image> *result,
                              std::string *err_msg) {
  std::vector<Magick::Image> frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception& e) {
    if (e.what()) *err_msg = e.what();
    return false;
  }
  if (frames.size() == 0) {
    fprintf(stderr, "No image found.");
    return false;
  }

  // Put together the animation from single frames. GIFs can have nasty
  // disposal modes, but they are handled nicely by coalesceImages()
  if (frames.size() > 1) {
    Magick::coalesceImages(result, frames.begin(), frames.end());
  } else {
    result->push_back(frames[0]);   // just a single still image.
  }

  const int img_width = (*result)[0].columns();
  const int img_height = (*result)[0].rows();
  const float width_fraction = (float)target_width / img_width;
  const float height_fraction = (float)target_height / img_height;
  if (fill_width && fill_height) {
    // Scrolling diagonally. Fill as much as we can get in available space.
    // Largest scale fraction determines that.
    const float larger_fraction = (width_fraction > height_fraction)
      ? width_fraction
      : height_fraction;
    target_width = (int) roundf(larger_fraction * img_width);
    target_height = (int) roundf(larger_fraction * img_height);
  }
  else if (fill_height) {
    // Horizontal scrolling: Make things fit in vertical space.
    // While the height constraint stays the same, we can expand to full
    // width as we scroll along that axis.
    target_width = (int) roundf(height_fraction * img_width);
  }
  else if (fill_width) {
    // dito, vertical. Make things fit in horizontal space.
    target_height = (int) roundf(width_fraction * img_height);
  }

  for (size_t i = 0; i < result->size(); ++i) {
    (*result)[i].scale(Magick::Geometry(target_width, target_height));
  }

  return true;
}

void DisplayAnimation(const FileInfo *file,
                      RGBMatrix *matrix, FrameCanvas *offscreen_canvas) {
  const tmillis_t duration_ms = (file->is_multi_frame
                                 ? file->params.anim_duration_ms
                                 : file->params.wait_ms);
  rgb_matrix::StreamReader reader(file->content_stream);
  int loops = file->params.loops;
  const tmillis_t end_time_ms = GetTimeInMillis() + duration_ms;
  const tmillis_t override_anim_delay = file->params.anim_delay_ms;
  for (int k = 0;
       (loops < 0 || k < loops)
         && !interrupt_received && !stopThread
         && GetTimeInMillis() < end_time_ms;
       ++k) {
    uint32_t delay_us = 0;
    while (!stopThread && !interrupt_received && GetTimeInMillis() <= end_time_ms
           && reader.GetNext(offscreen_canvas, &delay_us)) {
      const tmillis_t anim_delay_ms =
        override_anim_delay >= 0 ? override_anim_delay : delay_us / 1000;
      const tmillis_t start_wait_ms = GetTimeInMillis();
      offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas,
                                             file->params.vsync_multiple);
      const tmillis_t time_already_spent = GetTimeInMillis() - start_wait_ms;
      SleepMillis(anim_delay_ms - time_already_spent);
    }
    reader.Rewind();
  }
}

void image_handler(struct UploadedData req) {

  Magick::InitializeMagick(nullptr); 
  RGBMatrix::Options defaults;
  rgb_matrix::RuntimeOptions runtime_defaults;

  defaults.hardware_mapping = "adafruit-hat";
  defaults.rows              = 64; 
  defaults.cols              = 64;
  defaults.brightness        = 75;

  runtime_defaults.drop_privileges = 1; 
  runtime_defaults.gpio_slowdown   = 2; 
  runtime_defaults.drop_priv_user = getenv("SUDO_UID");
  runtime_defaults.drop_priv_group = getenv("SUDO_GID");
  
  char ** flags = NULL; 
  int num_flags = countWords(req.flags);
  flags = string_to_char_array(req.flags, num_flags); 
  
  matrix = RGBMatrix::CreateFromFlags(&num_flags, &flags, &defaults, &runtime_defaults);

  if (matrix == NULL) {
    return;
  }
  const bool fill_width   = false; 
  const bool fill_height  = false;
  bool do_center = true; 
  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas(); 

  const char *filename = req.file_path.c_str(); 
  FileInfo *file_info = NULL; 

  std::string err_msg; 
  std::vector<Magick::Image> image_sequence; 

  if (LoadImageAndScale(filename, matrix->width(), matrix->height(), 
                        fill_width, fill_height, &image_sequence, &err_msg)) {

    file_info = new FileInfo(); 
    file_info->params.anim_duration_ms = req.displayTime;
    file_info->params.wait_ms          = req.displayTime; 
    file_info->params.anim_delay_ms    = req.animFrameDelay; 
    file_info->params.loops            = req.loopCount;
    file_info->content_stream          = new rgb_matrix::MemStreamIO(); 
    file_info->is_multi_frame          = image_sequence.size() > 1; 
    rgb_matrix::StreamWriter out(file_info->content_stream);
    for (size_t i = 0; i < image_sequence.size(); i++) {
      const Magick::Image &img = image_sequence[i]; 
      int64_t delay_time_us; 
      if (file_info->is_multi_frame) {
        //std::cout << "multiframe" << std::endl; 
        delay_time_us = img.animationDelay() * 10000; // unit in 1/100s
      } else {
        //std::cout << "1 frame " << std::endl; 
        delay_time_us = file_info->params.wait_ms * 1000; // single image
      }
      if (delay_time_us <= 0) {
        delay_time_us = 100 * 1000; 
        //std::cerr << "default setting delay_time_us" << std::endl;
      }
      StoreInStream(img, delay_time_us, do_center, offscreen_canvas, &out);
      
    }
  }
  do {
    DisplayAnimation(file_info, matrix, offscreen_canvas);
    std::cerr << "stopThread " << stopThread;
    if (stopThread) {
      threadCount--;
      break;
    }
  } while (!interrupt_received && !stopThread);

  matrix->Clear(); 
  std::cerr << "Matrix Cleared\n"; 
  delete matrix; 

  return;
}
