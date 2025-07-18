# ESP32_LVGL_MusicPlayer
使用ESP32+LVGL实现音乐播放器UI及功能实现，使用VS CODE+platform，带歌词显示

演示地址：【ESP32+LVGL音乐播放器UI展示及功能实现，使用vscode+platform，带歌词显示】 https://www.bilibili.com/video/BV1cHKwzBEgz/?share_source=copy_web
<img width="1069" height="601" alt="image" src="https://github.com/user-attachments/assets/bf88908c-fa2f-4d8b-95a3-c15874a9e682" />

硬件：ESP32-WROOM开发板、MAX98357 I2S音频模块、micro SD卡模块、240*320TFT触摸屏（显示驱动为ST7789\触摸驱动XPT2046）

使用之前，记得先到ESP32_MusicPlayer_V4\lib\TFT_eSPI\User_Setup.h 这个文件里面修改TFT屏幕驱动（55行），RGB颜色顺序（77行）、颜色反向（117行）、引脚定义（166-176行）

SD卡SPI引脚、触摸SPI引脚、跟TFT SPI引脚使用的相同的引脚，只是CS控制引脚不同。
