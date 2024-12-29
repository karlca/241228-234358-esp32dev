/* Includes ------------------------------------------------------------------*/
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include "font_chinese.h"
#include <stdlib.h>
#include "time.h"
#include <WiFi.h>

// WiFi credentials
const char* ssid = "Xiaomi_799F";
const char* password = "18580631818";

// NTP Server
const char* ntpServer = "ntp.aliyun.com";
const long  gmtOffset_sec = 8 * 3600;  // 中国时区 UTC+8
const int   daylightOffset_sec = 0;

// 全局变量
UBYTE *BlackImage = NULL;
int lastMinute = -1;  // 用于追踪上次更新的分钟

// 显示时间的函数
void displayTime() {
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo)){
		printf("Failed to obtain time\n");
		return;
	}

	// 只有当分钟变化时才更新显示
	if(timeinfo.tm_min == lastMinute) {
		return;
	}
	lastMinute = timeinfo.tm_min;

	// 格式化时间字符串
	char timeStr[6];
	strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

	// 计算显示位置
	UWORD screen_width = EPD_2in13_V4_HEIGHT;   // 250
	UWORD screen_height = EPD_2in13_V4_WIDTH;   // 122
	UWORD x = (screen_width - strlen(timeStr) * Font24.Width) / 2;
	UWORD y = (screen_height - Font24.Height) / 2;

	static bool firstDisplay = true;
	if(firstDisplay) {
		// 第一次显示时进行完整初始化和清屏
		EPD_2in13_V4_Init();
		Paint_Clear(WHITE);
		firstDisplay = false;
	} else {
		// 后续更新使用快速刷新模式
		EPD_2in13_V4_Init_Fast();
		// 清除时间显示区域
		Paint_ClearWindows(x, y, x + Font24.Width * 5, y + Font24.Height, WHITE);
	}
	
	// 显示时间
	Paint_DrawString_EN(x, y, timeStr, &Font24, BLACK, WHITE);

	// 根据是否首次显示选择刷新模式
	if(firstDisplay) {
		EPD_2in13_V4_Display_Base(BlackImage);
	} else {
		EPD_2in13_V4_Display_Fast(BlackImage);
	}
}

/* Entry point ----------------------------------------------------------------*/
void setup()
{
	printf("EPD_2in13_V4_test Demo\r\n");
	DEV_Module_Init();

	// 连接WiFi
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		printf(".");
	}
	printf("\nWiFi connected\n");

	// 初始化时间
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	// 等待时间同步
	struct tm timeinfo;
	int retry = 0;
	while(!getLocalTime(&timeinfo) && retry < 10) {
		printf("Waiting for time sync...\n");
		delay(500);
		retry++;
	}

	// 创建图像缓存
	UWORD Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0)? (EPD_2in13_V4_WIDTH / 8 ): (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
	if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
		printf("Failed to apply for black memory...\r\n");
		while(1);
	}
	
	Paint_NewImage(BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
	Paint_SelectImage(BlackImage);
	Paint_Clear(WHITE);  // 确保初始背景为白色

	// 显示初始时间
	displayTime();
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
	displayTime();  // 检查并更新显示
	delay(1000);    // 每秒检查一次
	
	// 每小时重新同步一次时间
	static unsigned long lastSync = 0;
	if(millis() - lastSync > 3600000) {  // 3600000ms = 1小时
		configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
		lastSync = millis();
	}
}
