extern bool interrupt_received;
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
