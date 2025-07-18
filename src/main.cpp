#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <XPT2046_Touchscreen.h>
#include <Music.h>
#include "esp_task_wdt.h" // 确保包含头文件

/*函数声明*/
void UI_update();

XPT2046_Touchscreen xpt_touch = XPT2046_Touchscreen(TOUCH_CS); // 使用定义的触摸 CS 引脚
/*Don't forget to set Sketchbook location in File/Preferences to the path of your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 320;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 15];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  TS_Point p = xpt_touch.getPoint(); // 获取触摸点坐标
  bool touched = xpt_touch.touched();
  uint16_t touchX = 0, touchY = 0;

  if (touched)
  {
    // 将原始坐标映射到屏幕分辨率（例如 320x240）
    touchX = map(p.x, 320, 3700, 0, 240); // 映射到屏幕分辨率320,3700，为采集到最小和最大值，需要校准修改为实际值
    touchY = map(p.y, 300, 3820, 0, 320); // 映射到屏幕分辨率300,3820，为采集到最小和最大值，需要校准修改为实际值

    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
    Serial.print("Data x ");
    Serial.println(touchX);

    Serial.print("Data y ");
    Serial.println(touchY);
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
    data->point.x = 0;
    data->point.y = 0;
  }
}

// 多任务处理部分 ，内核0只处理音频播放，内核1处理歌词显示
TaskHandle_t audioTaskHandle = NULL;

// 音频播放任务
void audioTask(void *parameter)
{
  while (true)
  {
    Music_Loop(); // 音频循环处理

    vTaskDelay(2 / portTICK_PERIOD_MS); // 释放 CPU 时间片

    // 启用看门狗，需在循环中重置
    esp_task_wdt_reset();
  }
}

void setup()
{
  // 多任务处理部分，使用内核0处理音频播放
  xTaskCreatePinnedToCore(audioTask, "audioTask", 8192, NULL, configMAX_PRIORITIES - 1, &audioTaskHandle, 0);

  Serial.begin(9600); /* prepare for possible serial debug */

  /* 音频初始化*/
  Music_Init();

  delay(1000);

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");

  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  tft.begin();        /* TFT init */
  tft.setRotation(0); /* Landscape orientation, flipped */

  xpt_touch.begin();        // 初始化触摸屏
  xpt_touch.setRotation(0); // 设置与屏幕一致的方向

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 15);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();

  Serial.println("Setup done");

  /*初始化后更新页面显示信息*/
  lv_label_set_text(ui_MusicTitleLabel, currentTitle.c_str());   //  歌曲名
  lv_label_set_text(ui_MusicArtistLabel, currentArtist.c_str()); //  歌手名
  if (lrc_flag)
  {
    lv_label_set_text(ui_MusicLrcLabel, "歌词加载中..."); //  加载歌词
  }
  else
  {
    lv_label_set_text(ui_MusicLrcLabel, "暂无歌词"); //  加载歌词
  }
  //  显示播放时间+进度条
  if (duration != 0)
  {

    lv_img_set_angle(ui_citou, map(Music_GetCurrentPlayTime(), 0, duration, 60, -60)); // 设置磁头转动角度

    uint8_t minutes = Music_GetCurrentPlayTime() / 60;
    uint8_t seconds = Music_GetCurrentPlayTime() % 60;

    lv_label_set_text_fmt(ui_MusicTimeLabel1, "%02d:%02d", minutes, seconds);
    lv_label_set_text_fmt(ui_MusicTimeLabel2, "%02d:%02d", duration / 60, duration % 60);

    lv_bar_set_value(ui_Bar1, map(Music_GetCurrentPlayTime(), 0, duration, 0, 100), LV_ANIM_OFF);
  }
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  UI_update();        // 更新UI
  delay(5);
}

void UI_update()
{
  static bool play_executed = false;  // 播放执行标志位
  static bool pause_executed = false; // 暂停执行标志位

  if (Music_IsPlaying())
  {
    /* UI显示曲目名称，歌手，歌词，显示播放进度，播放时间，总时长，播放状态 */
    if (!play_executed)
    {
      _ui_flag_modify(ui_PlayButton, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);     // 隐藏播放按钮
      _ui_flag_modify(ui_PauseButton, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE); // 显示暂停按钮
      lv_img_set_angle(ui_citou, 0);                                               // 磁头转到播放位置
      HaibaoXuanzhuan_Animation(ui_haibao, 0);                                     // 胶片转动

      play_executed = true; // 设置执行标志
      pause_executed = false;
      Serial.println("播放执行标志位");
    }

    if (music_prev_i != music_i) // 播放歌曲索引与上一个歌曲索引不一致 ,重新解析歌曲信息和歌词
    {
      // 重置元数据
      duration = 0;
      durationPrinted = false;
      currentTitle = "";
      currentArtist = "";
      while (duration == 0) // 等待歌曲信息获取完毕 获取时长
      {
        Music_info();
      }
      parseLrcFile(musicFiles[music_i]); //  歌词解析

      // 更新索引追踪
      music_prev_i = music_i;
      // 立即更新UI显示
      lv_label_set_text(ui_MusicTitleLabel, currentTitle.c_str());   //  歌曲名
      lv_label_set_text(ui_MusicArtistLabel, currentArtist.c_str()); //  歌手名
      if (lrc_flag)
      {
        lv_label_set_text(ui_MusicLrcLabel, "歌词加载中..."); //  加载歌词
      }
      else
      {
        lv_label_set_text(ui_MusicLrcLabel, "暂无歌词"); //  加载歌词
      }
      Serial.println("更新索引和歌词");
    }

    // 显示播放歌词
    if (lrc_flag && Music_GetCurrentPlayTime() != temp_AudioCurrentTime) // lrc_flag 为找到歌词标志位，且播放时间值跟之前找到歌词时间值不一样，确保只显示一次歌词
    {

      for (uint8_t i = 0; i < lyricCount; i++)
      {
        if (Music_GetCurrentPlayTime() == lyrics[i].timestamp)
        {

          temp_AudioCurrentTime = Music_GetCurrentPlayTime();
          Serial.println(lyrics[i].lyric);
          lv_label_set_text(ui_MusicLrcLabel, lyrics[i].lyric.c_str());
          break;
        }
      }
    }
    if (!lrc_flag)
    {
      lv_label_set_text(ui_MusicLrcLabel, "暂无歌词"); //  加载歌词
    }
    //  显示播放时间+进度条
    if (duration != 0)
    {

      lv_img_set_angle(ui_citou, map(Music_GetCurrentPlayTime(), 0, duration, 60, -60)); // 设置磁头转动角度

      uint8_t minutes = Music_GetCurrentPlayTime() / 60;
      uint8_t seconds = Music_GetCurrentPlayTime() % 60;

      lv_label_set_text_fmt(ui_MusicTimeLabel1, "%02d:%02d", minutes, seconds);
      lv_label_set_text_fmt(ui_MusicTimeLabel2, "%02d:%02d", duration / 60, duration % 60);

      lv_bar_set_value(ui_Bar1, map(Music_GetCurrentPlayTime(), 0, duration, 0, 100), LV_ANIM_OFF);
    }
  }
  else
  {
    /* 曲目暂停时候，胶片停止转动，磁头停止位置，显示暂停状态；检查循环方式，切换下一个曲目或者停止； */
    if (!pause_executed)
    {
      _ui_flag_modify(ui_PauseButton, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);   // 隐藏暂停按钮
      _ui_flag_modify(ui_PlayButton, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE); // 显示播放按钮
      // lv_group_focus_obj(ui_bofang);                                          // 聚焦到播放按钮
      lv_img_set_angle(ui_citou, -200); // 磁头转到停止位置
      lv_anim_del_all();                // 停止胶片转动
      Serial.println("暂停执行标志位");
      pause_executed = true;
      /*如果不是按键暂停的，则检查循环方式*/
      if (pause_status != 1)
      {
        switch (currentPlayMode)
        {
        case LIST_LOOP:
          if (music_i < (fileCount - 1))
          {
            music_i++; // 切换到下一首
          }
          else
          {
            music_i = 0; // 回到列表开头
          }
          Music_PlayPath(musicFiles[music_i].c_str()); // 自动播放下一首
          Serial.println("列表循环播放下一首");
          break;

        case SINGLE_LOOP:
          Music_PlayPath(musicFiles[music_i].c_str()); // 重新播放当前曲目
          Serial.println("单曲循环播放");
          break;

        case RANDOM_PLAY:
          music_i = random(0, fileCount - 1); // 生成随机索引
          Music_PlayPath(musicFiles[music_i].c_str());
          Serial.println("随机播放");
          break;
        }
      }
    }

    play_executed = false; // 重置标志
  }
}