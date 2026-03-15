#include <stdio.h>
#include <Windows.h>
#include <conio.h>
#include <time.h>
#include<string.h>


// 宏定义：地图尺寸与按键控制
#define MAP_HEIGHT 20
#define MAP_WIDTH 40
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'

// 蛇身节点结构体
typedef struct {
    int x;
    int y;
} Snakenode;

// 食物结构体：包含坐标、类型及生长属性
typedef struct {
    int x;
    int y;
    int type; //1为普通 2为中级 3为高级
    int growth; //吃每一级食物能增长的长度

}Food;

// 静态障碍物结构体
typedef struct {
    int x;
    int y;
}Obstacle;

// 蛇结构体：核心数据模型
typedef struct {
    Snakenode snakenode[1000];// 身体数组（最大支持1000节）
    int length;//实时长度
    int speed;//实时速度
    int score;//实时成绩
    int add_length;//未消化量
    int boost_timer;//加速剩余时长
} Snake;

// 排行榜记录结构体
typedef struct {
    char name[20];
    int score;
}Record;

// 函数声明
void GotoXY(int x, int y);
void Hide();
int Menu();
void Help();
void About();
void InitMap();
void PrintFood();
int MoveSnake();
int IsCorrect();
void SpeedControl();
void SetDifficulty();
void ShowRank();
void print_invincible_food();
void chinese_info();



extern int obstacle_num;
extern int score_multiplier;
extern int if_invincible;
extern double invincible_time;