// Music.cpp
#include "Music.h"
#include <Audio.h>

// SD引脚定义
#define SD_Pin 5
// 设置I2S音频引脚
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// 音量
int volume = 10;

// 最大音乐数量
const int maxFiles = 50;
String musicFiles[maxFiles]; // 定义字符串数组存放音乐文件名称
int fileCount = 0;           // 当前音乐文件数量
String folder = "/music";    // 音乐文件夹路径
int music_i = 0;             // 当前播放索引
int music_prev_i = 0;        // 上一个播放索引

Audio audio;

bool durationPrinted = false; //
long duration = 0;            // 音频总时长

// 当前音频元数据
String currentTitle = "";  // 曲目名
String currentArtist = ""; // 歌手名
String currentAlbum = "";  // 专辑名

// LRC歌词相关变量
uint8_t lrc_m = 0;            // 分钟
uint8_t lrc_s = 0;            // 秒
String lrc_time;              // 时间字符串
boolean lrc_flag = 0;         // 是否存在歌词标志位
const size_t maxLyrics = 200; // 最大歌词行数
LyricEntry lyrics[maxLyrics]; // 歌词数组
int lyricCount = 0;           // 实际歌词行数
uint16_t temp_AudioCurrentTime = 1;

PlayMode currentPlayMode = LIST_LOOP; // 默认列表循环
uint8_t pause_status = 0;             // 暂停状态//

File file;

// 初始化音乐系统
void Music_Init()
{
  if (SD.begin(SD_Pin))
  {
    Serial.println("SD卡初始化成功");
  }
  else
  {
    Serial.println("SD卡初始化失败");
    ESP.restart();
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume); // 设置音量大小，范围0...21

  listMusicFiles(folder); // 列出音乐文件
  if (fileCount > 0)
  {
    String music_path = folder + "/" + musicFiles[music_i]; // 当前播放路径
    audio.connecttoFS(SD, music_path.c_str());              // 默认播放第一首
    while (duration == 0)                                   // 等待歌曲信息获取完毕 获取时长
    {
      Music_info();
    }
    parseLrcFile(musicFiles[music_i]); //  歌词解析
    pause_status = 1;                  // 初始化为暂停状态
    audio.pauseResume();               // 暂停播放
   
    Serial.printf("当前播放: %s\n", music_path.c_str());
  }
}

// 主循环处理音频播放
void Music_Loop()
{
  audio.loop();
}

// 列出指定目录下的所有音乐文件
void listMusicFiles(String dir)
{
  File root = SD.open(dir);
  if (!root)
  {
    Serial.println("无法打开目录");
    return;
  }

  while (true)
  {
    File file = root.openNextFile();
    if (!file)
      break;

    if (!file.isDirectory() && isMusicFile(file.name()))
    {
      Serial.print("发现音乐文件: ");
      Serial.println(file.name());

      if (fileCount < maxFiles)
      {
        musicFiles[fileCount++] = file.name();
      }
      else
      {
        Serial.println("达到最大音乐文件数上限");
        break;
      }
    }

    file.close();
  }

  root.close();
}

// 检查是否是音乐文件
bool isMusicFile(String name)
{
  return !name.startsWith(".") &&
         (name.endsWith(".mp3") || name.endsWith(".MP3") ||
          name.endsWith(".wma") || name.endsWith(".WMA") ||
          name.endsWith(".wav") || name.endsWith(".WAV"));
}

//  解析歌词文件
void parseLRC(const char *filename, LyricEntry *lyrics, int &lyricCount)
{
  lyricCount = 0;

  if (filename == nullptr || strlen(filename) == 0)
  {
    Serial.println("LRC 文件路径为空");
    return;
  }

  File file = SD.open(filename);
  if (!file)
  {
    Serial.printf("无法打开 LRC 文件: %s\n", filename);
    return;
  }

  while (file.available() && lyricCount < maxLyrics)
  {
    String line = file.readStringUntil('\n');
    line.trim();

    // 跳过空行和无时间标签行
    if (line.length() == 0 || !line.startsWith("["))
    {
      continue;
    }

    int startTag = 0;
    int endTag = line.indexOf("]");

    if (endTag == -1 || endTag < 3)
    {
      // 没有找到结束括号或格式不对
      continue;
    }

    String timestampStr = line.substring(startTag + 1, endTag);
    String lyricText = line.substring(endTag + 1);

    // 验证时间格式 mm:ss.xx 或 mm:ss.xx
    if (timestampStr.length() < 5 ||
        timestampStr[2] != ':' ||
        (timestampStr[5] != '.' && timestampStr[5] != ':'))
    {
      continue;
    }

    int timestamp = 0;
    uint8_t m1 = chartonumber(timestampStr[0]);
    uint8_t m2 = chartonumber(timestampStr[1]);
    uint8_t s1 = chartonumber(timestampStr[3]);
    uint8_t s2 = chartonumber(timestampStr[4]);

    timestamp = (m1 * 10 + m2) * 60 + s1 * 10 + s2;

    lyrics[lyricCount].timestamp = timestamp;
    lyrics[lyricCount].lyric = lyricText;
    lyricCount++;
  }

  file.close();
}
/*查找歌词文件是否存在，存在解析*/
void parseLrcFile(String MusicName)
{
  String music_path = folder + "/" + musicFiles[music_i];                      // 歌曲路径
  String lrc_path = music_path.substring(0, music_path.indexOf('.')) + ".lrc"; // 歌词路径
  if (SD.exists(music_path))
  {
    lrc_flag = 1;
    Serial.println("找到歌词文件");
    parseLRC(lrc_path.c_str(), lyrics, lyricCount);
  }
  else
  {
    lrc_flag = 0;
    Serial.println("没有找到歌词文件");
  }
}
// 打印歌词内容
void printLyrics(LyricEntry *lyrics, size_t lyricCount)
{
  for (size_t i = 0; i < lyricCount; i++)
  {
    Serial.printf("%d\n%s\n", lyrics[i].timestamp, lyrics[i].lyric.c_str());
  }
}

// 字符转数字
uint8_t chartonumber(char charnumber)
{
  if (charnumber >= '0' && charnumber <= '9')
  {
    return charnumber - '0';
  }
  return 0;
}

// ========== 播放控制模块 ==========

// 播放
void Music_Play()
{

  if (!audio.isRunning() && !audio.pauseResume())
  {
    String music_path = folder + "/" + musicFiles[music_i]; // 当前播放路径
    audio.connecttoFS(SD, music_path.c_str());
    Serial.print("当前曲目: ");
    Serial.println(musicFiles[music_i]);
  }
}
// 暂停
void Music_Pause()
{

  if (audio.isRunning())
  {
    audio.pauseResume();
  }
}

// 下一曲
void Music_Next()
{
  if (fileCount == 0)
    return; // 无文件可播

  music_i = (music_i + 1) % fileCount; // 循环到下一首
  audio.stopSong();                    // 切换曲目时候，先要停止歌曲，然后加个延迟，等I2S清空下缓冲区，然后再播放，否则会有I2S缓冲区满的错误
  delay(100);
  String music_path = folder + "/" + musicFiles[music_i]; // 当前播放路径
  audio.connecttoFS(SD, music_path.c_str());
  Serial.print("下一曲: ");
  Serial.println(musicFiles[music_i]);
}

// 上一曲
void Music_Prev()
{
  if (fileCount == 0)
    return; // 无文件可播

  music_i = (music_i - 1 + fileCount) % fileCount;        // 循环到上一首
  String music_path = folder + "/" + musicFiles[music_i]; // 当前播放路径
  audio.stopSong();
  delay(100);
  audio.connecttoFS(SD, music_path.c_str());
  Serial.print("上一曲: ");
  Serial.println(musicFiles[music_i]);
}

// 第一曲
void Music_First()
{
  if (fileCount == 0)
    return; // 无文件可播

  music_i = 0;                                            // 播放第一首
  String music_path = folder + "/" + musicFiles[music_i]; // 当前播放路径
  audio.stopSong();
  delay(100);
  audio.connecttoFS(SD, music_path.c_str());
  Serial.print("第一曲: ");
  Serial.println(musicFiles[music_i]);
}

// 最后一曲
void Music_Last()
{
  if (fileCount == 0)
    return; // 无文件可播

  music_i = fileCount - 1;                                // 播放最后一曲
  String music_path = folder + "/" + musicFiles[music_i]; // 当前播放路径
  audio.stopSong();
  delay(100);
  audio.connecttoFS(SD, music_path.c_str());
  Serial.print("最后一曲: ");
  Serial.println(musicFiles[music_i]);
}

bool Music_IsPlaying()
{
  return audio.isRunning();
}

// 显示音频总时长
// 如果播放时间值跟之前找到歌词时间值不一样，确保只显示一次总时长
/*获取音频总时长，曲目名称，歌手信息*/
void Music_info()
{
  if (!durationPrinted && audio.isRunning())
  {
    duration = audio.getAudioFileDuration();
    if (duration > 0)
    {
      Serial.printf("音频总时长：%d 秒,曲目名称：%s ,歌手：%s\n", duration, currentTitle.c_str(), currentArtist.c_str());

      // 如果标题为空，则显示文件名（不含扩展名）
      if (currentTitle.length() == 0)
      {
        String fileName = musicFiles[music_i];
        currentTitle = fileName.substring(0, fileName.lastIndexOf("."));
      }

      // 如果歌手为空，则显示 "未知艺术家"
      if (currentArtist.length() == 0)
      {
        currentArtist = "未知歌手";
      }

      durationPrinted = true;
    }
    else
    {
      Serial.println("音频时长获取失败（可能为 MP3 编码或未解析完成）");
      durationPrinted = false;
    }
  }
}

/*获取当前播放时长*/
uint32_t Music_GetCurrentPlayTime()
{
  return audio.getAudioCurrentTime();
}
/*播放指定路径下的音频*/
void Music_PlayPath(const char *path)
{
  audio.stopSong();
  delay(100);
  String music_path = folder + "/" + path;   // 当前播放路径
  audio.connecttoFS(SD, music_path.c_str()); // 默认播放
}

/*模式切换函数（可绑定到UI按钮）*/
void switchPlayMode()
{
  currentPlayMode = static_cast<PlayMode>(
      (currentPlayMode + 1) % 3); // 循环切换三种模式

  // 更新UI显示模式状态
  const char *modeText[] = {"单曲循环", "列表循环", "随机播放"};
  // lv_label_set_text(ui_modeLabel, modeText[currentPlayMode]);
}

/*设置音量*/
void setVolume(uint8_t volume)
{
  if (volume < 0)
  {
    volume = 0;
  }
  else if (volume > 21)
  {
    volume = 21;
  }
  audio.setVolume(volume);
}

/*获取音量值*/
uint8_t getVolume()
{
  return audio.getVolume();
}
