#define _CRT_SECURE_NO_WARNINGS
#include "snake(2).h"

int main() {
    srand((unsigned int)time(0));// 初始化伪随机数种子，确保每次游戏食物和障碍物位置不同
    while (1) {
        // 进入主菜单并获取玩家选择
        int choice = Menu();
        if (choice == 1) { // 开始游戏
            InitMap(); // 初始化地图、蛇及读取历史记录
            while (MoveSnake()); // 循环执行移动逻辑，直到游戏结束返回0
        }
        else if (choice == 2) { // 游戏帮助
            Help();
        }
        else if (choice == 3) { // 关于游戏
            About();
        }

        else if (choice == 4) { // 查看排行榜
            ShowRank();
        }
        else if (choice == 5) {
            SetDifficulty();
        }
        else if (choice == 0) { // 退出程序
            break;
        }
    }
    return 0;
}
