#ifndef MUSIC_H
#define MUSIC_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

enum PlayMode
{
    SINGLE_LOOP, // 单曲循环
    LIST_LOOP,   // 列表循环
    RANDOM_PLAY  // 随机播放
};

// 全局变量声明
extern int volume;          // 音量
extern int fileCount;           // 当前音乐文件总数量
extern String folder;
extern String currentTitle;
extern String currentArtist;
extern String currentAlbum;
extern long duration;
extern bool durationPrinted;
extern int music_i;         // 当前播放文件索引
extern int music_prev_i;    // 上一个播放文件索引
extern int lyricCount;      // 歌词数量
extern boolean lrc_flag; // 是否存在歌词标志位
extern uint8_t lrc_m;       // 歌词分钟
extern uint8_t lrc_s;       // 歌词秒
extern boolean lrc_flag;    // 是否存在歌词标志位
extern struct LyricEntry lyrics[]; // ✅ 结构体数组
extern uint16_t temp_AudioCurrentTime;
extern String musicFiles[];
extern PlayMode currentPlayMode;
extern uint8_t pause_status;    // 暂停状态标志位

void Music_Init();
void Music_Loop();
// void Music_PlayPause();
// void Music_Next();
// void Music_Prev();

// 函数声明
bool isMusicFile(String name);
void listMusicFiles(String dir);
void parseLRC(const char *filename, struct LyricEntry *lyrics, size_t &lyricCount);
void printLyrics(struct LyricEntry *lyrics, size_t lyricCount);
uint8_t chartonumber(char charnumber);
bool Music_IsPlaying();
void Music_info();
void parseLrcFile(String MusicName);
uint32_t Music_GetCurrentPlayTime();
void Music_PlayPath(const char *path);

// 结构体定义
struct LyricEntry
{
    uint16_t timestamp;
    String lyric;
};

#ifdef __cplusplus
extern "C"
{
#endif

    void Music_Play();
    void Music_Pause();
    void Music_Next();
    void Music_Prev();
    void Music_First();
    void Music_Last();
    void setVolume(uint8_t volume);
    uint8_t getVolume();
    void switchPlayMode();

#ifdef __cplusplus
}
#endif

#endif // MUSIC_H