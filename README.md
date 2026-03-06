#define ST7735_DRIVER      // Driver cho màn hình 0.96
#define ST7735_GREENTAB160x80 

#define TFT_WIDTH  80
#define TFT_HEIGHT 160

// Định nghĩa chân cắm cho ESP32-S3 SuperMini
#define TFT_MOSI 6  // Chân SDA trên màn hình
#define TFT_SCLK 5  // Chân SCL trên màn hình
#define TFT_CS   9  // Chân CS trên màn hình
#define TFT_DC   8  // Chân DC trên màn hình
#define TFT_RST  7  // Chân RES trên màn hình

#define LOAD_GLCD
#define SPI_FREQUENCY  27000000
