#include "led-matrix.h" 
#include "string_manipulation.hpp"

#include <Magick++.h>
#include <magick/image.h>
#include <unistd.h>

volatile bool interrupt_received; 
using rgb_matrix::RGBMatrix; 
using rgb_matrix::Canvas; 
using rgb_matrix::FrameCanvas;
using ImageVector = std::vector<Magick::Image>; 

RGBMatrix * matrix; 
static ImageVector LoadImageAndScaleImage(const char *filename,
                                          int target_width,
                                          int target_height) {
  ImageVector result;

  ImageVector frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception &e) {
    if (e.what())
      fprintf(stderr, "LoadImageAndScale: %s\n", e.what());
    return result;
  }

  if (frames.empty()) {
    fprintf(stderr, "No image found.");
    return result;
  }

  // Animated images have partial frames that need to be put together
  if (frames.size() > 1) {
    Magick::coalesceImages(&result, frames.begin(), frames.end());
  } else {
    result.push_back(frames[0]); // just a single still image.
  }

  for (Magick::Image &image : result) {
    image.scale(Magick::Geometry(target_width, target_height));
  }

  return result;
}

void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas) {
  const int offset_x = 0, offset_y = 0;  // If you want to move the image.
  // Copy all the pixels to the canvas.
  for (size_t y = 0; y < image.rows(); ++y) {
    for (size_t x = 0; x < image.columns(); ++x) {
      const Magick::Color &c = image.pixelColor(x, y);
      if (c.alphaQuantum() < 256) {
        canvas->SetPixel(x + offset_x, y + offset_y,
                         ScaleQuantumToChar(c.redQuantum()),
                         ScaleQuantumToChar(c.greenQuantum()),
                         ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
}

void ShowAnimatedImage(const ImageVector &images, RGBMatrix *matrix) {
  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
  while (!interrupt_received) {
    for (const auto &image : images) {
      if (interrupt_received) {
        break;
      }
      offscreen_canvas->Clear();
      CopyImageToCanvas(image, offscreen_canvas);
      offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
      usleep(image.animationDelay() * 1000);  // 1/10s converted to usec
    }
  }
}

void clear_matrix() {
  matrix->Clear(); 
  delete matrix; 
  return;
}
  

void image_handler(std::string flags, int num_flags) { 
  
  Magick::InitializeMagick(nullptr);
  
  RGBMatrix::Options defaults; 
  rgb_matrix::RuntimeOptions runtime_defaults; 
  
  defaults.hardware_mapping = "adafruit-hat";
  defaults.rows              = 64; 
  defaults.cols              = 64;
  defaults.brightness        = 75;

  runtime_defaults.drop_privileges = 1; 
  runtime_defaults.gpio_slowdown   = 2; 
  runtime_defaults.drop_priv_user  = getenv("SUDO_UID"); 
  runtime_defaults.drop_priv_group = getenv("SUDO_GID");

  char ** char_flags = NULL; 
  char_flags = string_to_char_array(flags, num_flags); 
  
  matrix = RGBMatrix::CreateFromFlags(&num_flags, &char_flags, 
                        &defaults, &runtime_defaults); 

  if (matrix == NULL) {
    return; 
  } 
  /* PrintMatrixFlags(stderr, defaults, runtime_defaults); */

  ImageVector images = LoadImageAndScaleImage("/tmp/uploaded_file", matrix->width(), matrix->height()); 

  switch (images.size()) {
  case 0:   // failed to load image.
    break;
  case 1:   // Simple example: one image to show
    CopyImageToCanvas(images[0], matrix);
    while (!interrupt_received) sleep(1000);
    break;
  default:  // More than one image: this is an animation.
    std::cout << "ShowAnimatedImage" << std::endl;
    ShowAnimatedImage(images, matrix);
    break;
  }

  matrix->Clear(); 
  delete matrix; 
  return; 
}
