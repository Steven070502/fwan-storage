"""
俄罗斯方块 - Windows 控制台版
操作方式：
  ← →   左右移动
  ↑      旋转
  ↓      加速下落
  空格   硬降（直接到底）
  P      暂停/继续
  Q      退出游戏
  R      重新开始（游戏结束后）
"""

import os
import sys
import time
import random
import threading
import msvcrt

# ── 游戏常量 ──────────────────────────────────────────────
COLS = 10
ROWS = 20
EMPTY = '.'

# 关卡配置：(名称, 初始下落间隔秒, 每消行速度增量)
LEVELS = [
    ("新手",   0.80, 0.010),
    ("普通",   0.55, 0.015),
    ("困难",   0.35, 0.020),
    ("地狱",   0.18, 0.025),
    ("无尽",   0.10, 0.005),
]

# 方块形状 (每种颜色对应一种方块)
TETROMINOES = {
    'I': [['....', 'XXXX', '....', '....'],
          ['..X.', '..X.', '..X.', '..X.']],
    'O': [['XX', 'XX']],
    'T': [['XXX', '.X.'],
          ['.X.', '.XX', '.X.'],
          ['.X.', 'XXX'],
          ['.X.', 'XX.', '.X.']],
    'S': [['.XX', 'XX.'],
          ['X..', 'XX.', '.X.']],
    'Z': [['XX.', '.XX'],
          ['.X.', 'XX.', 'X..']],
    'J': [['X..', 'XXX'],
          ['XX.', 'X..', 'X..'],
          ['XXX', '..X'],
          ['.X.', '.X.', '.XX']],
    'L': [['..X', 'XXX'],
          ['X..', 'X..', 'XX.'],
          ['XXX', 'X..'],
          ['XX.', '.X.', '.X.']],
}

# ANSI 颜色代码
COLORS = {
    'I': '\033[96m',   # 亮青
    'O': '\033[93m',   # 亮黄
    'T': '\033[95m',   # 亮紫
    'S': '\033[92m',   # 亮绿
    'Z': '\033[91m',   # 亮红
    'J': '\033[94m',   # 亮蓝
    'L': '\033[33m',   # 橙
    'G': '\033[90m',   # 幽灵（灰）
}
RESET = '\033[0m'
BOLD  = '\033[1m'

BLOCK  = '██'
GHOST  = '░░'
EMPTY_CELL = '· '
BORDER = '│'
H_LINE = '─'


# ── 键盘输入（Windows）────────────────────────────────────
class KeyReader:
    """非阻塞键盘读取"""
    def __init__(self):
        self._key = None
        self._lock = threading.Lock()
        t = threading.Thread(target=self._run, daemon=True)
        t.start()

    def _run(self):
        while True:
            if msvcrt.kbhit():
                ch = msvcrt.getwch()
                if ch in ('\x00', '\xe0'):   # 方向键前缀
                    ch2 = msvcrt.getwch()
                    key = {
                        'H': 'UP', 'P': 'DOWN',
                        'K': 'LEFT', 'M': 'RIGHT'
                    }.get(ch2, None)
                else:
                    key = ch.upper()
                with self._lock:
                    self._key = key
            time.sleep(0.01)

    def get(self):
        with self._lock:
            k = self._key
            self._key = None
        return k


# ── 俄罗斯方块核心逻辑 ───────────────────────────────────
class Tetris:
    def __init__(self, level_idx=0):
        self.level_idx = level_idx
        self.level_name, self.drop_interval, self.speed_inc = LEVELS[level_idx]
        self.reset()

    def reset(self):
        self.board = [[None] * COLS for _ in range(ROWS)]
        self.score = 0
        self.lines = 0
        self.combo = 0
        self.level = 1
        self.interval = LEVELS[self.level_idx][1]
        self.game_over = False
        self.paused = False
        self.piece = self._new_piece()
        self.next_piece = self._new_piece()

    def _new_piece(self):
        kind = random.choice(list(TETROMINOES.keys()))
        shapes = TETROMINOES[kind]
        return {
            'kind': kind,
            'shapes': shapes,
            'rot': 0,
            'x': COLS // 2 - len(shapes[0][0]) // 2,
            'y': 0,
        }

    def _shape(self, piece=None, rot=None):
        p = piece or self.piece
        r = rot if rot is not None else p['rot']
        return p['shapes'][r % len(p['shapes'])]

    def _fits(self, piece, dx=0, dy=0, rot=None):
        shape = self._shape(piece, rot)
        for row_i, row in enumerate(shape):
            for col_i, cell in enumerate(row):
                if cell != 'X':
                    continue
                nx = piece['x'] + col_i + dx
                ny = piece['y'] + row_i + dy
                if nx < 0 or nx >= COLS or ny >= ROWS:
                    return False
                if ny >= 0 and self.board[ny][nx] is not None:
                    return False
        return True

    def _ghost_y(self):
        dy = 0
        while self._fits(self.piece, dy=dy+1):
            dy += 1
        return self.piece['y'] + dy

    def move(self, dx):
        if self._fits(self.piece, dx=dx):
            self.piece['x'] += dx

    def rotate(self):
        new_rot = (self.piece['rot'] + 1) % len(self.piece['shapes'])
        for kick in [0, -1, 1, -2, 2]:
            if self._fits(self.piece, dx=kick, rot=new_rot):
                self.piece['x'] += kick
                self.piece['rot'] = new_rot
                return

    def soft_drop(self):
        if self._fits(self.piece, dy=1):
            self.piece['y'] += 1
            self.score += 1
        else:
            self._lock_piece()

    def hard_drop(self):
        dy = 0
        while self._fits(self.piece, dy=dy+1):
            dy += 1
        self.score += dy * 2
        self.piece['y'] += dy
        self._lock_piece()

    def _lock_piece(self):
        shape = self._shape()
        for row_i, row in enumerate(shape):
            for col_i, cell in enumerate(row):
                if cell == 'X':
                    nx = self.piece['x'] + col_i
                    ny = self.piece['y'] + row_i
                    if ny < 0:
                        self.game_over = True
                        return
                    self.board[ny][nx] = self.piece['kind']
        cleared = self._clear_lines()
        self._update_score(cleared)
        self.piece = self.next_piece
        self.next_piece = self._new_piece()
        if not self._fits(self.piece):
            self.game_over = True

    def _clear_lines(self):
        full = [i for i, row in enumerate(self.board) if all(c is not None for c in row)]
        for i in full:
            del self.board[i]
            self.board.insert(0, [None] * COLS)
        return len(full)

    def _update_score(self, cleared):
        pts = [0, 100, 300, 500, 800]
        if cleared:
            self.combo += 1
            base = pts[cleared] * self.level
            combo_bonus = 50 * self.combo * self.level
            self.score += base + combo_bonus
            self.lines += cleared
            self.level = self.lines // 10 + 1
            self.interval = max(0.05, LEVELS[self.level_idx][1] - self.lines * self.speed_inc)
        else:
            self.combo = 0

    def gravity_tick(self):
        if not self._fits(self.piece, dy=1):
            self._lock_piece()
        else:
            self.piece['y'] += 1


# ── 渲染器 ───────────────────────────────────────────────
def hide_cursor():
    sys.stdout.write('\033[?25l')

def show_cursor():
    sys.stdout.write('\033[?25h')

def goto(row, col):
    sys.stdout.write(f'\033[{row};{col}H')

def clear_screen():
    os.system('cls')

def enable_ansi():
    """Windows 10+ 开启 ANSI 转义支持"""
    import ctypes
    kernel32 = ctypes.windll.kernel32
    kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)

def color(kind):
    return COLORS.get(kind, '')

def render(game: Tetris):
    """全量渲染一帧，使用绝对定位避免闪烁"""
    ghost_y = game._ghost_y()
    shape = game._shape()
    piece = game.piece

    lines_buf = []

    # 顶部边框
    lines_buf.append(f'╔{"══" * COLS}╗')

    for row_i in range(ROWS):
        row_str = BORDER
        for col_i in range(COLS):
            cell = game.board[row_i][col_i]
            # 当前方块
            is_piece = False
            is_ghost = False
            for r_i, row in enumerate(shape):
                for c_i, ch in enumerate(row):
                    if ch == 'X':
                        py = piece['y'] + r_i
                        px = piece['x'] + c_i
                        if py == row_i and px == col_i:
                            is_piece = True
                        if ghost_y + r_i == row_i and px == col_i and not is_piece:
                            is_ghost = True

            if is_piece:
                row_str += f"{color(piece['kind'])}{BLOCK}{RESET}"
            elif is_ghost:
                row_str += f"{COLORS['G']}{GHOST}{RESET}"
            elif cell:
                row_str += f"{color(cell)}{BLOCK}{RESET}"
            else:
                row_str += f"\033[90m{EMPTY_CELL}{RESET}"
        row_str += BORDER
        lines_buf.append(row_str)

    lines_buf.append(f'╚{"══" * COLS}╝')

    # 侧边信息（嵌入在游戏区右侧）
    side = []
    side.append(f"{BOLD}俄罗斯方块{RESET}")
    side.append(f"关卡: {game.level_name}")
    side.append("")
    side.append(f"分数: {BOLD}{game.score}{RESET}")
    side.append(f"等级: {BOLD}{game.level}{RESET}")
    side.append(f"行数: {BOLD}{game.lines}{RESET}")
    side.append(f"连消: {BOLD}{game.combo}{RESET}")
    side.append("")
    side.append("─ 下一个 ─")
    # 渲染 next piece
    n_shape = game._shape(game.next_piece, 0)
    for r in n_shape:
        row_render = ""
        for c in r:
            if c == 'X':
                row_render += f"{color(game.next_piece['kind'])}{BLOCK}{RESET}"
            else:
                row_render += "  "
        side.append(row_render)
    side.append("")
    side.append("─ 操作 ─")
    side.append("← → 移动")
    side.append("↑   旋转")
    side.append("↓   加速")
    side.append("空格 硬降")
    side.append("P   暂停")
    side.append("Q   退出")
    if game.paused:
        side.append("")
        side.append(f"\033[93m【已暂停】{RESET}")

    # 输出（绝对定位）
    goto(1, 1)
    for i, line in enumerate(lines_buf):
        goto(i + 1, 1)
        sys.stdout.write(line)
    for i, s in enumerate(side):
        goto(i + 1, COLS * 2 + 5)
        sys.stdout.write(s + '          ')  # 清除残留字符

    sys.stdout.flush()


def render_game_over(game: Tetris):
    mid_row = ROWS // 2
    goto(mid_row, 4)
    sys.stdout.write(f"\033[41m{BOLD} 游戏结束！ {RESET}")
    goto(mid_row + 1, 4)
    sys.stdout.write(f"\033[41m{BOLD} 得分: {game.score:>5} {RESET}")
    goto(mid_row + 2, 4)
    sys.stdout.write(f"\033[41m{BOLD} 行数: {game.lines:>5} {RESET}")
    goto(mid_row + 3, 4)
    sys.stdout.write(f"\033[41m{BOLD} R=重来 Q=退出 {RESET}")
    sys.stdout.flush()


# ── 关卡选择界面 ─────────────────────────────────────────
def select_level():
    clear_screen()
    hide_cursor()
    selected = 0
    while True:
        goto(1, 1)
        lines = []
        lines.append(f"{BOLD}╔══════════════════════════╗{RESET}")
        lines.append(f"{BOLD}║    俄罗斯方块 - 关卡选择   ║{RESET}")
        lines.append(f"{BOLD}╚══════════════════════════╝{RESET}")
        lines.append("")
        for i, (name, interval, inc) in enumerate(LEVELS):
            spd = f"速度: {1/interval:.1f}格/秒  加速: {inc*100:.1f}%/行"
            if i == selected:
                lines.append(f"  \033[97m\033[44m▶ {i+1}. {name:<4}  {spd}{RESET}")
            else:
                lines.append(f"    {i+1}. \033[90m{name:<4}  {spd}{RESET}")
        lines.append("")
        lines.append("  ↑↓ 选择关卡    Enter 确认    Q 退出")
        for i, l in enumerate(lines):
            goto(i + 1, 1)
            sys.stdout.write(l + '                    ')
        sys.stdout.flush()

        if msvcrt.kbhit():
            ch = msvcrt.getwch()
            if ch in ('\x00', '\xe0'):
                ch2 = msvcrt.getwch()
                if ch2 == 'H' and selected > 0:
                    selected -= 1
                elif ch2 == 'P' and selected < len(LEVELS) - 1:
                    selected += 1
            elif ch == '\r':
                return selected
            elif ch.upper() == 'Q':
                show_cursor()
                sys.exit(0)
        time.sleep(0.05)


# ── 主循环 ───────────────────────────────────────────────
def main():
    # 开启 Windows ANSI 支持
    try:
        enable_ansi()
    except Exception:
        pass

    level_idx = select_level()
    clear_screen()
    hide_cursor()

    game = Tetris(level_idx)
    keys = KeyReader()

    last_drop = time.time()

    try:
        while True:
            now = time.time()

            # 处理按键
            key = keys.get()
            if key:
                if key == 'Q':
                    break
                elif key == 'P':
                    game.paused = not game.paused
                elif not game.paused and not game.game_over:
                    if key == 'LEFT':
                        game.move(-1)
                    elif key == 'RIGHT':
                        game.move(1)
                    elif key == 'UP':
                        game.rotate()
                    elif key == 'DOWN':
                        game.soft_drop()
                        last_drop = now
                    elif key == ' ':
                        game.hard_drop()
                        last_drop = now
                elif game.game_over:
                    if key == 'R':
                        game.reset()
                        last_drop = now

            # 重选关卡：游戏结束时按 L
            if game.game_over and key == 'L':
                level_idx = select_level()
                clear_screen()
                game = Tetris(level_idx)
                last_drop = time.time()

            # 自动下落
            if not game.paused and not game.game_over:
                if now - last_drop >= game.interval:
                    game.gravity_tick()
                    last_drop = now

            # 渲染
            render(game)
            if game.game_over:
                render_game_over(game)

            time.sleep(0.02)

    finally:
        show_cursor()
        clear_screen()
        print(f"感谢游玩！最终得分: {game.score}  行数: {game.lines}")


if __name__ == '__main__':
    main()