#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "OV7670.h"
#include "BMP.h"
#include <TFT_eSPI.h> // TFT 屏幕库
#include <HTTPClient.h>
#include <ArduinoJson.h> // 用于解析JSON数据
#include<WebServer.h>

WiFiClient espClient;

// ====== 摄像头 GPIO 定义 ======
const int SIOD = 21; // SDA
const int SIOC = 22; // SCL
const int VSYNC = 34;
const int HREF = 35;
const int XCLK = 32;
const int PCLK = 33;
const int D0 = 27;
const int D1 = 17;
const int D2 = 16;
const int D3 = 15;
const int D4 = 14;
const int D5 = 13;
const int D6 = 12;
const int D7 = 4;

// ====== WIFI 配置 ======
const char *g_ssid = "MyWIFI";
const char *g_password = "Aa123123";
WiFiClient g_espclient;

// ====== 服务器配置 ======
const char *NODE_SERVER = "192.168.97.65";
const int NODE_PORT = 8000;
WebServer server(80);

// 摄像头实例
OV7670 *camera;
unsigned char bmpHeader[BMP::headerSize];

// TFT 显示屏
TFT_eSPI tft = TFT_eSPI(); // 创建 TFT 屏幕对象



// 连接 WiFi
void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(g_ssid, g_password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
}

// 发送给服务器
void sendToServer()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi未连接，无法上传");
    return;
  }

  Serial.println("开始上传图片到服务器...");
  Serial.println("本机IP: " + WiFi.localIP().toString());
  Serial.println("目标服务器: http://192.168.97.65:8000/upload");

  // 先测试网络连接
  WiFiClient testClient;
  if (!testClient.connect("192.168.97.65", 8000))
  {
    Serial.println("WiFi 本机 IP   : " + WiFi.localIP().toString());
    Serial.println("WiFi 网关      : " + WiFi.gatewayIP().toString());
    Serial.println("子网掩码        : " + WiFi.subnetMask().toString());
    Serial.println("无法连接到服务器 192.168.97.65:8000");
    Serial.println("请检查：");
    Serial.println("1. 电脑防火墙是否阻止了8000端口");
    Serial.println("2. 后端服务器是否运行在0.0.0.0:8000而不是localhost:8000");
    Serial.println("3. ESP32和电脑是否在同一网络");
    return;
  }
  testClient.stop();
  Serial.println("网络连接测试成功");

  WiFiClient client;
  HTTPClient http;

  // 确保获取最新的一帧
  camera->oneFrame();

  http.begin(client, "http://192.168.97.65:8000/upload");
  http.addHeader("Content-Type", "application/octet-stream");
  http.addHeader("User-Agent", "ESP32-Camera");
  http.setTimeout(15000); // 15秒超时

  // 计算完整图片大小（包含BMP头）
  int headerSize = BMP::headerSize;
  int imageDataSize = camera->xres * camera->yres * 2;
  int totalSize = headerSize + imageDataSize;

  // 创建完整的BMP数据
  uint8_t *fullImage = (uint8_t *)malloc(totalSize);
  if (fullImage == NULL)
  {
    Serial.println("内存分配失败");
    http.end();
    return;
  }

  // 复制BMP头和图像数据
  memcpy(fullImage, bmpHeader, headerSize);
  memcpy(fullImage + headerSize, camera->frame, imageDataSize);

  Serial.printf("准备上传 %d 字节数据 (头部:%d + 图像:%d)\n", totalSize, headerSize, imageDataSize);

  int httpResponseCode = http.POST(fullImage, totalSize);

  Serial.printf("POST响应码: %d\n", httpResponseCode);

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("服务器响应: " + response);

    if (httpResponseCode == 200)
    {
      Serial.println("✅ 图片上传成功！");
    }
    else
    {
      Serial.printf("❌ 上传失败，HTTP响应码: %d\n", httpResponseCode);
    }
  }
  else
  {
    Serial.printf("❌ 请求失败，错误码: %d\n", httpResponseCode);

    // 解释常见错误码
    switch (httpResponseCode)
    {
    case -1:
      Serial.println("连接失败 - 服务器无响应或网络问题");
      break;
    case -2:
      Serial.println("发送请求头失败");
      break;
    case -3:
      Serial.println("发送请求体失败");
      break;
    case -4:
      Serial.println("服务器连接丢失");
      break;
    case -5:
      Serial.println("读取响应失败");
      break;
    case -6:
      Serial.println("连接超时");
      break;
    default:
      Serial.println("未知错误");
    }

    Serial.println("\n🔧 故障排除建议：");
    Serial.println("1. 确认后端服务器运行命令：");
    Serial.println("   node server.js 或类似命令");
    Serial.println("2. 确认服务器监听地址是 0.0.0.0:8000 而不是 localhost:8000");
    Serial.println("3. 检查Windows防火墙是否阻止了8000端口");
    Serial.println("4. 在电脑浏览器测试：http://192.168.97.65:8000");
  }

  free(fullImage);
  http.end();
}

// ========== 摄像头图像 → TFT 屏幕 显示 ==========
void displayCameraFrame()
{
  uint16_t color;
  int index = 0;

  tft.startWrite();
  tft.setAddrWindow(0, 0, 128, 128); // 设置 TFT 128x128 显示区域

  for (int y = 0; y < 128; y++)
  {
    for (int x = 0; x < 128; x++)
    {
      int src_x = (x * camera->xres) / 128; // 映射 x
      int src_y = (y * camera->yres) / 128; // 映射 y
      index = (src_y * camera->xres + src_x) * 2;

      uint8_t byte1 = camera->frame[index];     // 低字节
      uint8_t byte2 = camera->frame[index + 1]; // 高字节
      color = (byte2 << 8) | byte1;             // RGB565 颜色格式

      tft.pushColor(color);
    }
  }
  tft.endWrite();
}

// ========== 设置 ==========
void setup()
{
  Serial.begin(115200);
  Serial.println();
  connectWifi();

    // 注册路由
  server.on("/take-photo", HTTP_GET, []() {
    Serial.println("收到 /take-photo 请求，开始上传...");
    sendToServer();  // 调用你之前写的上传函数
    server.send(200, "text/plain", "ESP32 已触发上传");
  });

  server.begin();
  Serial.println("HTTP 服务器已启动");

  if (WiFi.status() == WL_CONNECTED)
  {

    // 初始化摄像头
    camera = new OV7670(OV7670::Mode::QQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
    BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
  }

  // 初始化 TFT 屏幕
  tft.init();
  tft.setRotation(0); // 方向
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.println("ESP32 Camera Ready!");
}
//===========实时流功能=========
void sendStreamToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClient client;
  HTTPClient http;
  
  // 获取最新帧
  camera->oneFrame();
  
  // 发送到流端点
  http.begin(client, "http://192.168.97.65:8000/stream-upload");
  http.addHeader("Content-Type", "application/octet-stream");
  http.setTimeout(5000); // 5秒超时，比普通上传短
  
  // 计算图片大小
  int headerSize = BMP::headerSize;
  int imageDataSize = camera->xres * camera->yres * 2;
  int totalSize = headerSize + imageDataSize;
  
  // 创建完整BMP数据
  uint8_t *fullImage = (uint8_t *)malloc(totalSize);
  if (fullImage != NULL) {
    memcpy(fullImage, bmpHeader, headerSize);
    memcpy(fullImage + headerSize, camera->frame, imageDataSize);
    
    int httpResponseCode = http.POST(fullImage, totalSize);
    
    // 简单的成功/失败日志，避免过多输出
    if (httpResponseCode != 200 && httpResponseCode > 0) {
      Serial.printf("Stream upload failed: %d\n", httpResponseCode);
    }
    
    free(fullImage);
  }
  
  http.end();
}
//===========实时流功能=========

// ========== 主循环 ==========
void loop()
{

  camera->oneFrame();   // 采集一帧摄像头画面
  displayCameraFrame(); // 显示图像到 TFT
 server.handleClient();
  // 实时流上传（高频率）
  static unsigned long lastStream = 0;
  if (millis() - lastStream > 200) { // 每200ms发送一次，约5FPS
    sendStreamToServer();
    lastStream = millis();
  }

  static unsigned long lastUpload = 0;
  if (millis() - lastUpload > 30000)
  { // 30秒间隔
    sendToServer();
    lastUpload = millis();
  }
  delay(100); // 控制帧率
}

