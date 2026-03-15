#define _CRT_SECURE_NO_WARNINGS
#include "snake(2).h"

// 全局变量定义
#include <string.h>
int high_score = 0;
Snakenode spec_food;
Snake snake;
Food food;
char now_Dir = RIGHT;
Obstacle obstacle[50];
Record player[10];
int obstacle_num = 8;
int score_multiplier = 1;  //根据难度设定得分倍率  默认分数倍率 1 倍
int if_invincible = 0;  // 检测是否处于无敌状态
double invincible_time = 0;  //无敌状态秒数
int first = 1;

void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void SetDifficulty() {
    system("cls");
    GotoXY(35, 8);  printf("=== Select Difficulty ===");
    GotoXY(35, 10); printf("1. Easy   (5  Obstacles, 1x Score)");
    GotoXY(35, 11); printf("2. Normal (15 Obstacles, 2x Score)");
    GotoXY(35, 12); printf("3. HELL   (30 Obstacles, 5x Score)");

    char ch = _getch();
    switch (ch) {
    case '1': obstacle_num = 5;  score_multiplier = 1; break;
    case '2': obstacle_num = 15; score_multiplier = 2; break;
    case '3': obstacle_num = 30; score_multiplier = 5; break;
    default:  obstacle_num = 8;  score_multiplier = 1; break;
    }

}
// 控制台光标定位：将打印位置移动到坐标(x, y)
void GotoXY(int x, int y) {
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cor = { (short)x, (short)y };
    SetConsoleCursorPosition(hout, cor);
}

// 隐藏控制台光标：提升视觉效果，避免闪烁
void Hide() {
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cor_info = { 1, 0 };
    SetConsoleCursorInfo(hout, &cor_info);
}

// 主菜单逻辑
int Menu() {
    system("cls");
    GotoXY(35, 10); printf("=== Welcome to Snake Game ===");
    GotoXY(40, 12); printf("1. Start Game");
    GotoXY(40, 14); printf("2. Help");
    GotoXY(40, 16); printf("3. About");
    GotoXY(40, 18); printf("4. ShowRank");
    GotoXY(40, 20); printf("5.SetDifficulty");
    GotoXY(40, 22); printf("0. Exit");
    Hide();
    char ch = _getch();
    if (ch == '1') return 1;
    if (ch == '2') return 2;
    if (ch == '3') return 3;
    if (ch == '4') return  4;
    if (ch == '5')return 5;
    if (ch == '0') return 0;
    return -1;
}

//关于游戏
void About() {
    system("cls");
    GotoXY(30, 10); printf("HDU - Programming Practice Case");
    GotoXY(30, 12); printf("Snake Game Console Version");
    GotoXY(30, 14); printf("Press any key to return...");
    _getch();
}

//获取帮助
void Help() {
    system("cls");
    GotoXY(35, 10); printf("Controls: W-Up, S-Down, A-Left, D-Right");
    GotoXY(35, 12); printf("Game ends if you hit the wall or yourself.");
    GotoXY(35, 14); printf("Press any key to return...");
    _getch();
}

// 初始化游戏地图、蛇、障碍物及排行榜
void InitMap() {


    // 1. 排行榜读档：从本地 record.txt 读取前十名
    FILE* fp = fopen("record.txt", "r");
    if (fp == NULL) {
        for (int i = 0; i < 10; i++) {
            strcpy(player[i].name, "empty");
            player[i].score = 0;
        }
    }
    else {
        for (int i = 0; i < 10; i++) {
            fscanf(fp, "%s %d", player[i].name, &player[i].score);
        }
        fclose(fp);
    }


    system("cls");
    Hide();

    // 2. 初始化蛇属性
    snake.snakenode[0].x = MAP_WIDTH / 2;
    snake.snakenode[0].y = MAP_HEIGHT / 2;
    snake.length = 3;
    snake.speed = 250;
    snake.score = 0;
    now_Dir = RIGHT;
    snake.length = 3;
    snake.speed = 250;
    snake.score = 0;

    //显式归零，防止读取到内存残留的旧数据
    snake.boost_timer = 0;
    snake.add_length = 0;

    if_invincible = 0;
    invincible_time = 0;

    // 绘制初始蛇身
    for (int i = 0; i < snake.length; i++) {
        snake.snakenode[i].y = snake.snakenode[0].y;
        snake.snakenode[i].x = snake.snakenode[0].x - i;
        GotoXY(snake.snakenode[i].x, snake.snakenode[i].y);
        printf(i == 0 ? "@" : "o");
    }
    // 绘制地图上下边界
    for (int i = 0; i < MAP_WIDTH; i++) {
        GotoXY(i, 0); printf("-");
        GotoXY(i, MAP_HEIGHT - 1); printf("-");
    }
    //绘制地图左右边界
    for (int i = 1; i < MAP_HEIGHT - 1; i++) {
        GotoXY(0, i); printf("|");
        GotoXY(MAP_WIDTH - 1, i); printf("|");
    }

    // 3. 随机生成8个静态障碍物 (#)
    for (int i = 0; i < obstacle_num; i++) {
        int flag = 1;
        while (flag) {
            flag = 0;
            //确保障碍物在地图内
            obstacle[i].x = rand() % (MAP_WIDTH - 2) + 1;
            obstacle[i].y = rand() % (MAP_HEIGHT - 2) + 1;

            //确保障碍物不在蛇身上
            for (int k = 0; k < snake.length; k++) {
                if (snake.snakenode[k].x == obstacle[i].x && snake.snakenode[k].y == obstacle[i].y) {
                    flag = 1;
                    break;
                }
            }
            //确保障碍物不重叠
            if (!flag) {
                for (int j = 0; j < i; j++) {
                    if (obstacle[j].x == obstacle[i].x && obstacle[j].y == obstacle[i].y) {
                        flag = 1;
                        break;
                    }
                }
            }
        }
    }
    //打印障碍物
    for (int i = 0; i < obstacle_num; i++) {
        GotoXY(obstacle[i].x, obstacle[i].y);
        printf("#");
    }


    PrintFood();
    print_invincible_food(); // 打印无敌道具
}

//随机刷新食物
void PrintFood() {
    int flag = 1;
    while (flag) {
        flag = 0;
        food.x = rand() % (MAP_WIDTH - 2) + 1;
        food.y = rand() % (MAP_HEIGHT - 2) + 1;
        int possibility = rand() % 100;//以70%:20%:10%的比例刷新普通($) 中级(*) 高级(+)三种食物
        if (possibility <= 70) {
            food.type = 1;
            food.growth = 1;
        }
        else if (possibility <= 90) {
            food.type = 2;
            food.growth = 3;
        }
        else if (possibility < 100) {
            food.type = 3;
            food.growth = 5;
        }

        //确保食物不刷新在蛇身上
        for (int k = 0; k < snake.length; k++) {
            if (snake.snakenode[k].x == food.x && snake.snakenode[k].y == food.y) {
                flag = 1; break;
            }
        }

        //确保食物不和障碍物重叠
        if (!flag) {
            for (int j = 0; j < obstacle_num; j++) {
                if (obstacle[j].x == food.x && obstacle[j].y == food.y) {
                    flag = 1;
                    break;
                }
            }
        }
    }
    //打印食物
    GotoXY(food.x, food.y);
    if (food.type == 1) {
        printf("$");
    }
    else if (food.type == 2) {
        printf("*");
    }
    else if (food.type == 3) {
        printf("+");
    }
}

// 显示本地排行榜
void ShowRank() {
    system("cls");
    GotoXY(35, 5);
    printf("=== TOP 10 ===");
    //打开record.txt读入最新数据
    FILE* fp = fopen("record.txt", "r");
    if (fp) {
        for (int i = 0; i < 10; i++) {
            fscanf(fp, "%s %d", player[i].name, &player[i].score);
        }
        fclose(fp);
    }
    for (int i = 0; i < 10; i++) {
        GotoXY(35, 7 + i); //逐行输出
        printf("No.%d %s %d", i + 1, player[i].name, player[i].score);
    }
    GotoXY(35, 19);
    printf("Press any key to return the Menu");
    _getch();
}

// 核心移动与逻辑处理函数
int MoveSnake() {

    int is_cut = 0;// 标志位：记录本回合是否发生自撞截断
    // 备份当前尾巴位置，用于后续擦除残影或生长新节点

    // 1. 数据移位：蛇身每一节向前挪动一格
    Snakenode temp = snake.snakenode[snake.length - 1];
    for (int i = snake.length - 1; i >= 1; i--) {
        snake.snakenode[i] = snake.snakenode[i - 1];
    }

    // 2. 接收输入：处理方向按键
    if (_kbhit()) {
        char input = _getch();
        if (input == UP && now_Dir != DOWN) now_Dir = UP;
        else if (input == DOWN && now_Dir != UP) now_Dir = DOWN;
        else if (input == LEFT && now_Dir != RIGHT) now_Dir = LEFT;
        else if (input == RIGHT && now_Dir != LEFT) now_Dir = RIGHT;
    }

    // 3. 更新蛇头坐标
    switch (now_Dir) {
    case UP:    snake.snakenode[0].y--; break;
    case DOWN:  snake.snakenode[0].y++; break;
    case LEFT:  snake.snakenode[0].x--; break;
    case RIGHT: snake.snakenode[0].x++; break;
    }
	// 4. 拓展点：无敌状态穿墙逻辑
    if (if_invincible) {
        // 水平穿墙
        if (snake.snakenode[0].x <= 0)
            snake.snakenode[0].x = MAP_WIDTH - 2;
        else if (snake.snakenode[0].x >= MAP_WIDTH - 1)
            snake.snakenode[0].x = 1;

        // 垂直穿墙
        if (snake.snakenode[0].y <= 0)
            snake.snakenode[0].y = MAP_HEIGHT - 2;
        else if (snake.snakenode[0].y >= MAP_HEIGHT - 1)
            snake.snakenode[0].y = 1;
    }
    // 5. 拓展点：自撞截断逻辑
    // 如果撞到身体，不结束游戏，而是将碰撞点后的身躯全部截除
    for (int i = 1; i < snake.length; i++) {
        if (snake.snakenode[i].x == snake.snakenode[0].x && snake.snakenode[i].y == snake.snakenode[0].y) {
            is_cut = 1;
            for (int j = i; j <= snake.length - 1; j++) {
                GotoXY(snake.snakenode[j].x, snake.snakenode[j].y);
                printf(" ");
            }
            snake.length = i;// 缩短蛇身长度
            break;
        }
    }

    print_invincible_food(); // 打印无敌道具

    // 6. 死亡判定：撞墙或撞障碍物
    if (!IsCorrect()) {

        GotoXY(40, 12); printf("GAME OVER!");
        GotoXY(40, 14); printf("Final Score: %d", snake.score);
        int rank = -1;
        for (int i = 0; i < 10; i++) {
            if (snake.score > player[i].score) {
                rank = i;
                break;
            }
        }

        // 检查是否进入前十名
        if (rank != -1) {
            system("cls");
            printf("You entered the Top 10! Name: ");
            char name[20] = { 0 };
            scanf("%s", name);


            for (int i = 9; i > rank; i--) {//从后向前挪动位置，腾出空间
                player[i] = player[i - 1];
            }
            strcpy(player[rank].name, name);
            player[rank].score = snake.score;

            // 存入文件：持久化保存排行榜
            FILE* fp = fopen("record.txt", "w");
            if (fp) {
                for (int i = 0; i < 10; i++) {
                    fprintf(fp, "%s %d\n", player[i].name, player[i].score);
                }
                fclose(fp);
            }
            else {
                GotoXY(40, 16); printf("Press any key to see Leaderboard");
                _getch(); // 等待玩家按键
            }
            ShowRank();//输入名字 即刻展示成绩

        }
        else {
            GotoXY(40, 16); printf("Press any key to return the Menu");
            _getch();
        }
        return 0;// 返回0结束 MoveSnake 循环
    }

    // 7. 渲染图形：绘制新头并把原来的头变身为身体('o')
    if (snake.boost_timer > 0) {
        SetColor(12);//淡红色 表示加速中
    }
    else {
        SetColor(7);
    }
    GotoXY(snake.snakenode[0].x, snake.snakenode[0].y); printf("@");
    //身体设置绿色
    SetColor(10);
    if (snake.length > 1) {
        GotoXY(snake.snakenode[1].x, snake.snakenode[1].y); printf("o");
    }
    SetColor(7);//改回白色 继续接下来的操作

    if (snake.snakenode[0].x == spec_food.x && snake.snakenode[0].y == spec_food.y) {
        if_invincible = 1;
        invincible_time = 5;
        // 打印无敌道具
    }


    // 8. 进食判定+吃到无敌道具
    if (snake.snakenode[0].x == food.x && snake.snakenode[0].y == food.y) {
        snake.boost_timer += 15;//获得加速量
        snake.add_length += food.growth;
        PrintFood();
    }
    //平滑消化食物 不会导致隔空连接最末节点和被撞节点
    if (!is_cut) {
        if (snake.add_length == 0) {
            GotoXY(temp.x, temp.y); printf(" ");// 无消化任务，正常擦除尾迹
        }
        else {
            snake.score += score_multiplier;// 每消化出一节，增加积分 难度越高消化单位食物所得分越高
            snake.length++;// 蛇身实际变长
            snake.snakenode[snake.length - 1] = temp;// 尾迹变为新身体
            snake.add_length--;// 未消化量减一
        }
    }
    else {
        GotoXY(temp.x, temp.y);
		printf(" ");//发生自撞截断时，直接擦除尾迹，不进行消化增长
    }
    SpeedControl();
    GotoXY(MAP_WIDTH + 2, 5);
    printf("Current Score: %d", snake.score);//实时显示成绩

    GotoXY(MAP_WIDTH + 2, 20);
    printf("Invincible_time : %.3f", invincible_time);

    int info_x = MAP_WIDTH + 2;
    int info_y = 7;
    int final_speed = snake.speed;
    if (snake.boost_timer > 0) {
        GotoXY(info_x, info_y);
        if (snake.boost_timer % 2 == 0) {
            printf(">> BOOSTING! <<");
        }
        else printf("   BOOSTING!   ");
        final_speed /= 2;
        snake.boost_timer -= 1;
    }
    else {
        GotoXY(info_x, info_y);
        printf("               ");
    }
    Sleep(final_speed);

    if (invincible_time > 0) {
        // final_speed 是毫秒，除以 1000.0 转换为秒
        invincible_time -= (final_speed / 1000.0);
    }

    // 强制归零防止浮点数微小误差（如 0.00001）
    if (invincible_time <= 0.001) {
        invincible_time = 0;
        if_invincible = 0;
    }

    return 1;

}

// 边界与障碍物碰撞检测
int IsCorrect() {
        //只要是无敌状态，统统判定为合法（不死）
        if (if_invincible) {
            return 1;
        }
    
        if (snake.snakenode[0].x <= 0 || snake.snakenode[0].x >= MAP_WIDTH - 1 ||
            snake.snakenode[0].y <= 0 || snake.snakenode[0].y >= MAP_HEIGHT - 1) return 0;
        //for (int i = 1; i < snake.length; i++) {
        //    if (snake.snakenode[i].x == snake.snakenode[0].x &&
        //        snake.snakenode[i].y == snake.snakenode[0].y) return 0;
        //}
        for (int i = 0; i < obstacle_num; i++) {
            if (snake.snakenode[0].x == obstacle[i].x && snake.snakenode[0].y == obstacle[i].y) {
                return 0;
            }
        }
       


    //无敌状态检测
    
    return 1;
}

// 自动难度分级：随长度增加而提速
void SpeedControl() {
    if (snake.length >= 20) snake.speed = 80;
    else if (snake.length >= 10) snake.speed = 150;
}

void print_invincible_food() {
    // 为了保证无敌道具不会生成太多次
    int percent = rand() % 1000;
    if (first) {
        percent = 1;
        first = 0;
    }//保证第一次一定会生成
    if (percent >= 10) return;

    int f = 0;
    while (!f) {
        f = 1;
        // 防止无敌道具生成在 障碍物 高级道具 和 蛇身上
        spec_food.x = rand() % (MAP_WIDTH - 2) + 1;
        spec_food.y = rand() % (MAP_HEIGHT - 2) + 1;

        if (spec_food.x == food.x && spec_food.y == food.y) {
            f = 0;
            break;
        }//不让特殊道具生成在食物上

        for (int k = 0; k < snake.length; k++) {
            if (snake.snakenode[k].x == spec_food.x && snake.snakenode[k].y == spec_food.y) {
                f = 0;
                break;
            }
        }//检测蛇

        for (int j = 0; j < obstacle_num; j++) {
            if (obstacle[j].x == spec_food.x && obstacle[j].y == spec_food.y) {
                f = 0;
                break;
            }
        }//检测障碍物
    }

    GotoXY(spec_food.x, spec_food.y);
    printf("g");
}