import tkinter as tk
from tkinter import ttk, messagebox
import time


class Position:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y

    def __repr__(self):
        return f"({self.x}, {self.y})"

    def __hash__(self):  # 用于在集合/字典中使用 Position
        return hash((self.x, self.y))


class ChessGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("骑士巡游可视化")

        # 棋盘参数
        self.board_size = 8
        self.cell_size = 60
        self.width = self.cell_size * self.board_size
        self.height = self.cell_size * self.board_size

        # 状态数据
        self.start_pos = None
        self.path = []  # 保存完整巡游路径
        self.current_path_index = -1  # 当前显示到哪一步
        self.is_playing = False  # 是否自动播放
        self.playback_job = None  # after() 的任务 ID
        self.board_matrix = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]  # 0: 未访, 步数: 已访
        self.solve_timeout_seconds = 8  # 回溯模式超时，避免卡死
        self._timeout_hit = False       # 标记是否因超时中断

        # 骑士的 8 种走法
        self.moves = [
            (1, 2), (1, -2), (-1, 2), (-1, -2),
            (2, 1), (-2, 1), (2, -1), (-2, -1),
        ]

        # 算法选择
        self.algorithm_var = tk.StringVar(value="warnsdorff")  # 默认 Warnsdorff

        self._setup_ui()

    def _setup_ui(self):
        # 主容器
        main_app_frame = ttk.Frame(self.root, padding="10")
        main_app_frame.pack(fill=tk.BOTH, expand=True)

        # 棋盘画布
        self.canvas = tk.Canvas(
            main_app_frame,
            width=self.width,
            height=self.height,
            bg="lightgray",
            highlightthickness=0,
        )
        self.canvas.pack(padx=10, pady=10, side=tk.TOP)
        self.canvas.bind("<Button-1>", self.on_board_click)
        self.draw_board()

        # 底部控制区域
        control_panel_frame = ttk.Frame(main_app_frame, padding="8")
        control_panel_frame.pack(side=tk.BOTTOM, fill=tk.X, pady=5)

        # 提示文字
        self.lbl_info = ttk.Label(
            control_panel_frame,
            text="点击棋盘选择起点",
            font=("Arial", 10),
        )
        self.lbl_info.pack(side=tk.TOP, pady=(0, 6))

        # 防止程序同步进度条时触发回调
        self._is_updating_progress_scale = False

        # 按钮+进度容器
        action_bar_frame = ttk.Frame(control_panel_frame)
        action_bar_frame.pack(side=tk.BOTTOM, fill=tk.X)

        # 算法选择
        algo_frame = ttk.LabelFrame(action_bar_frame, text="算法选择", padding="6")
        algo_frame.pack(side=tk.LEFT, padx=6, pady=2, fill=tk.Y)
        ttk.Radiobutton(
            algo_frame,
            text="Warnsdorff",
            variable=self.algorithm_var,
            value="warnsdorff",
            command=self.reset_board,
        ).pack(anchor=tk.W)
        ttk.Radiobutton(
            algo_frame,
            text="回溯",
            variable=self.algorithm_var,
            value="backtracking",
            command=self.reset_board,
        ).pack(anchor=tk.W)

        # 巡游控制
        tour_control_frame = ttk.LabelFrame(action_bar_frame, text="巡游控制", padding="6")
        tour_control_frame.pack(side=tk.LEFT, padx=6, pady=2, fill=tk.Y)

        self.btn_start = ttk.Button(tour_control_frame, text="开始", command=self.start_animation)
        self.btn_start.grid(row=0, column=0, padx=3, pady=2, sticky="ew")

        self.btn_pause = ttk.Button(tour_control_frame, text="暂停", command=self.pause_animation)
        self.btn_pause.grid(row=0, column=1, padx=3, pady=2, sticky="ew")

        self.btn_prev = ttk.Button(tour_control_frame, text="上一步", command=self.prev_step)
        self.btn_prev.grid(row=1, column=0, padx=3, pady=2, sticky="ew")

        self.btn_next = ttk.Button(tour_control_frame, text="下一步", command=self.next_step)
        self.btn_next.grid(row=1, column=1, padx=3, pady=2, sticky="ew")

        self.btn_back_to_start = ttk.Button(tour_control_frame, text="回到起点", command=self.go_to_start)
        self.btn_back_to_start.grid(row=2, column=0, columnspan=2, padx=3, pady=2, sticky="ew")

        self.btn_reset = ttk.Button(tour_control_frame, text="重置", command=self.reset_board)
        self.btn_reset.grid(row=0, column=2, rowspan=3, padx=6, pady=2, sticky="ns")

        for col in range(3):
            tour_control_frame.columnconfigure(col, weight=1)

        # 进度条
        progress_frame = ttk.LabelFrame(action_bar_frame, text="进度", padding="6")
        progress_frame.pack(side=tk.LEFT, padx=6, pady=2, fill=tk.BOTH, expand=True)

        self.progress_label = ttk.Label(progress_frame, text="步数: 0/0")
        self.progress_label.pack(side=tk.TOP, anchor=tk.W)

        self.progress_scale = ttk.Scale(
            progress_frame,
            from_=0,
            to=self.board_size * self.board_size - 1,
            orient=tk.HORIZONTAL,
            command=self.on_progress_scale_change,
        )
        self.progress_scale.pack(side=tk.BOTTOM, fill=tk.X, expand=True, padx=6)
        self.progress_scale.set(0)  # 初始值
        self.progress_scale.config(state=tk.DISABLED)  # 未找到路径前禁用

        # 初始状态下禁用操作按钮
        self.refresh_controls()

    def refresh_controls(self):
        """根据播放状态启用/禁用控件。"""
        has_path = bool(self.path)
        at_start = self.current_path_index <= 0
        at_end = has_path and self.current_path_index >= len(self.path) - 1

        self.btn_start.config(state=tk.NORMAL if has_path and not self.is_playing and not at_end else tk.DISABLED)
        self.btn_pause.config(state=tk.NORMAL if has_path and self.is_playing else tk.DISABLED)
        self.btn_prev.config(state=tk.NORMAL if has_path and not self.is_playing and not at_start else tk.DISABLED)
        self.btn_next.config(state=tk.NORMAL if has_path and not self.is_playing and not at_end else tk.DISABLED)
        self.btn_back_to_start.config(state=tk.NORMAL if has_path and not self.is_playing and not at_start else tk.DISABLED)
        self.btn_reset.config(state=tk.NORMAL)
        self.progress_scale.config(state=tk.NORMAL if has_path else tk.DISABLED)

    def set_tour_controls_state(self, state):
        """临时锁定/解锁巡游相关按钮。"""
        if state == tk.DISABLED:
            for btn in (self.btn_start, self.btn_pause, self.btn_prev, self.btn_next, self.btn_back_to_start):
                btn.config(state=tk.DISABLED)
            self.progress_scale.config(state=tk.DISABLED)
        else:
            self.refresh_controls()

    def draw_board(self):
        self.canvas.delete("all")
        colors = ["#F0D9B5", "#B58863"]  # 经典木色

        for row in range(self.board_size):
            for col in range(self.board_size):
                x1 = col * self.cell_size
                y1 = row * self.cell_size
                x2 = x1 + self.cell_size
                y2 = y1 + self.cell_size
                color = colors[(row + col) % 2]
                self.canvas.create_rectangle(x1, y1, x2, y2, fill=color, outline="")

    def on_board_click(self, event):
        if self.is_playing:
            return

        col = event.x // self.cell_size
        row = event.y // self.cell_size

        if 0 <= col < self.board_size and 0 <= row < self.board_size:
            self.reset_board(full_reset=False)  # 保留算法选择
            self.start_pos = Position(row, col)
            self.draw_knight(self.start_pos, color="red")  # 红色标记起点
            self.lbl_info.config(text=f"起点: ({row+1}, {col+1})，正在计算路径...")
            self.set_tour_controls_state(tk.DISABLED)  # 计算期间禁用操作
            self.root.update_idletasks()  # 刷新 UI 显示提示

            # --- 求解路径 ---
            start_time = time.time()
            success = self.solve_knight_tour_wrapper()  # 按当前算法求解
            end_time = time.time()

            if success:
                # 如需闭合且可回到起点，则追加起点成为第 65 步，用绿色线收尾
                if self._is_closed_move(self.path[-1], self.start_pos):
                    self.path.append(self.start_pos)
                total_steps = len(self.path)
                print(f"Path calculated in {end_time - start_time:.4f} seconds.")
                self.lbl_info.config(text=f"起点: ({row+1}, {col+1})。路径找到（共 {total_steps} 步），可开始播放。")
                self.current_path_index = 0
                self.progress_scale.config(to=total_steps - 1, state=tk.NORMAL)
                self.draw_current_step()
                self.set_tour_controls_state(tk.NORMAL)
            else:
                if self._timeout_hit and self.algorithm_var.get() == "backtracking":
                    self.lbl_info.config(text=f"起点: ({row+1}, {col+1})。回溯算法超时，建议改用 Warnsdorff。")
                    messagebox.showwarning("超时", "回溯搜索耗时过长，已中止。可改用 Warnsdorff（推荐）或换一个起点。")
                else:
                    self.lbl_info.config(text=f"起点: ({row+1}, {col+1})。未找到可行路径。")
                    messagebox.showerror("错误", "该起点和算法未找到骑士巡游解。")
                self.set_tour_controls_state(tk.DISABLED)  # 无路径则禁用操作

    def draw_knight(self, pos, color="black", step_num=None):
        # 清理之前的骑士与编号
        self.canvas.delete("knight_symbol")
        self.canvas.delete("step_number")

        cx = pos.y * self.cell_size + self.cell_size // 2
        cy = pos.x * self.cell_size + self.cell_size // 2

        if step_num is not None:
            self.canvas.create_text(cx, cy, text=str(step_num), font=("Arial", 14, "bold"), fill=color, tags="step_number")
        else:
            self.canvas.create_text(cx, cy, text="♞", font=("Arial", 30), fill=color, tags="knight_symbol")

    def draw_line(self, pos1, pos2, color="blue", arrow=None, tags="tour_line"):
        x1 = pos1.y * self.cell_size + self.cell_size // 2
        y1 = pos1.x * self.cell_size + self.cell_size // 2
        x2 = pos2.y * self.cell_size + self.cell_size // 2
        y2 = pos2.x * self.cell_size + self.cell_size // 2
        self.canvas.create_line(x1, y1, x2, y2, fill=color, width=2, arrow=arrow, tags=tags)

    def reset_board(self, full_reset=True):
        self.pause_animation()
        self.start_pos = None
        self.path = []
        self.current_path_index = -1
        self.board_matrix = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]
        self.draw_board()
        self.canvas.delete("knight_symbol")
        self.canvas.delete("tour_line")
        self.canvas.delete("step_number")
        self.progress_scale.config(to=self.board_size * self.board_size, state=tk.DISABLED)
        self.progress_scale.set(0)
        self.progress_label.config(text="步数: 0/0")
        self.lbl_info.config(text="点击棋盘选择起点")
        self.set_tour_controls_state(tk.DISABLED)

    # ================= 求解算法 =================

    def get_valid_moves(self, pos, visited_board):
        """获取某位置所有合法跳跃点（考虑 visited_board）。"""
        valid = []
        for dx, dy in self.moves:
            nx, ny = pos.x + dx, pos.y + dy
            if 0 <= nx < self.board_size and 0 <= ny < self.board_size and visited_board[nx][ny] == 0:
                valid.append(Position(nx, ny))
        return valid

    def warnsdorff_sort(self, current_pos, moves, visited_board):
        """Warnsdorff 规则：优先访问后续可走步数最少的位置。"""
        moves_with_degree = []
        for m in moves:
            temp_board = [row[:] for row in visited_board]
            temp_board[m.x][m.y] = 1
            degree = len(self.get_valid_moves(m, temp_board))
            moves_with_degree.append((degree, m))

        moves_with_degree.sort(key=lambda x: (x[0], x[1].x, x[1].y))
        return [m for _, m in moves_with_degree]

    def _is_closed_move(self, current_pos, start_pos):
        """当前点是否能一步回到起点。"""
        dx = abs(current_pos.x - start_pos.x)
        dy = abs(current_pos.y - start_pos.y)
        return (dx, dy) in {(1, 2), (2, 1)}

    def warnsdorff_greedy(self, start_pos):
        """Warnsdorff 贪心版本，快速给出路径；失败则返回 False。"""
        board = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]
        path = []

        current = Position(start_pos.x, start_pos.y)
        board[current.x][current.y] = 1
        path.append(current)

        for step in range(2, self.board_size * self.board_size + 1):
            moves = self.get_valid_moves(current, board)
            if not moves:
                return False, []

            # 选择 Warnsdorff 度最小的下一步
            def degree(pos):
                return len(self.get_valid_moves(pos, board))

            moves.sort(key=lambda p: (degree(p), p.x, p.y))
            nxt = moves[0]

            board[nxt.x][nxt.y] = step
            path.append(nxt)
            current = nxt

        return True, path

    def solve_knight_tour_wrapper(self):
        """根据当前选择的算法求解路径。"""
        self.board_matrix = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]
        self.path = []
        self.is_playing = False
        if self.playback_job:
            self.root.after_cancel(self.playback_job)
        self._timeout_hit = False

        current_algorithm = self.algorithm_var.get()
        start_time = time.time()

        if current_algorithm == "warnsdorff":
            success, greedy_path = self.warnsdorff_greedy(self.start_pos)

            # 贪心失败则回退到带 Warnsdorff 排序的 DFS，确保尽量找到解
            if not success:
                fallback_board = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]
                fallback_path = []
                success = self._dfs_tour(
                    self.start_pos,
                    1,
                    fallback_board,
                    fallback_path,
                    use_warnsdorff=True,
                    start_time=start_time,
                    timeout=self.solve_timeout_seconds,
                )
                if success:
                    self.board_matrix = fallback_board
                    self.path = fallback_path
                return success

            self.board_matrix = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]
            for idx, p in enumerate(greedy_path, start=1):
                self.board_matrix[p.x][p.y] = idx
            self.path = greedy_path
            return True
        else:
            return self._dfs_tour(
                self.start_pos,
                1,
                self.board_matrix,
                self.path,
                use_warnsdorff=False,
                start_time=start_time,
                timeout=self.solve_timeout_seconds,
            )

    def _dfs_tour(self, current_pos, step_count, visited_board, current_path, use_warnsdorff=False, start_time=None, timeout=None):
        """
        深度优先的骑士巡游求解。
        visited_board: 记录访问状态（0 未访问，步数 已访问）
        current_path: 已走路径
        """
        if timeout is not None and start_time is not None:
            if time.time() - start_time > timeout:
                self._timeout_hit = True
                return False

        visited_board[current_pos.x][current_pos.y] = step_count
        current_path.append(current_pos)

        if step_count == self.board_size * self.board_size:
            return True

        moves = self.get_valid_moves(current_pos, visited_board)
        if use_warnsdorff:
            sorted_moves = self.warnsdorff_sort(current_pos, moves, visited_board)
        else:
            # 回溯模式也用 Warnsdorff 次序，减少搜索分支
            sorted_moves = self.warnsdorff_sort(current_pos, moves, visited_board)

        for next_move in sorted_moves:
            if self._dfs_tour(
                next_move,
                step_count + 1,
                visited_board,
                current_path,
                use_warnsdorff,
                start_time,
                timeout,
            ):
                return True

            # 回溯：撤销并继续
            current_path.pop()
            visited_board[next_move.x][next_move.y] = 0

        return False

    # ================= 动画与显示 =================

    def draw_current_step(self):
        self.canvas.delete("tour_line")
        self.canvas.delete("step_number")
        self.canvas.delete("knight_symbol")

        if not self.path:
            self.progress_label.config(text="步数: 0/0")
            return

        # 画出已走路径
        for i in range(self.current_path_index):
            p1 = self.path[i]
            p2 = self.path[i + 1]
            is_final_jump = (
                len(self.path) >= 2
                and i == len(self.path) - 2
                and self.current_path_index == len(self.path) - 1
                and self.path[-1] == self.path[0]
            )
            line_color = "green" if is_final_jump else "blue"
            self.draw_line(p1, p2, color=line_color, tags="tour_line")
            cx = p1.y * self.cell_size + self.cell_size // 2
            cy = p1.x * self.cell_size + self.cell_size // 2
            self.canvas.create_text(cx, cy, text=str(i + 1), font=("Arial", 10), fill="darkblue", tags="step_number")

        # 当前骑士位置
        current_pos = self.path[self.current_path_index]
        self.draw_knight(current_pos, color="black")

        # 当前步号
        cx = current_pos.y * self.cell_size + self.cell_size // 2
        cy = current_pos.x * self.cell_size + self.cell_size // 2
        self.canvas.create_text(cx, cy, text=str(self.current_path_index + 1), font=("Arial", 14, "bold"), fill="black", tags="step_number")

        total_steps = len(self.path)
        self.progress_label.config(text=f"步数: {self.current_path_index + 1}/{total_steps}")
        self._is_updating_progress_scale = True
        self.progress_scale.set(self.current_path_index)
        self._is_updating_progress_scale = False

        self.refresh_controls()

    def start_animation(self):
        if not self.path or self.is_playing:
            return
        self.is_playing = True
        self.refresh_controls()
        self.animate_next_step()

    def pause_animation(self):
        if self.playback_job:
            self.root.after_cancel(self.playback_job)
        self.is_playing = False
        self.refresh_controls()

    def animate_next_step(self):
        if not self.is_playing or self.current_path_index >= len(self.path) - 1:
            self.is_playing = False
            self.refresh_controls()
            return

        self.current_path_index += 1
        self.draw_current_step()
        self.playback_job = self.root.after(100, self.animate_next_step)  # 100ms 间隔

    def next_step(self):
        self.pause_animation()
        if not self.path:
            return
        if self.current_path_index < len(self.path) - 1:
            self.current_path_index += 1
            self.draw_current_step()
            self.refresh_controls()

    def prev_step(self):
        self.pause_animation()
        if not self.path:
            return
        if self.current_path_index > 0:
            self.current_path_index -= 1
            self.draw_current_step()
            self.refresh_controls()

    def go_to_start(self):
        """跳回路径起点。"""
        self.pause_animation()
        if not self.path:
            return
        self.current_path_index = 0
        self.draw_current_step()
        self.refresh_controls()

    def on_progress_scale_change(self, value):
        if self._is_updating_progress_scale:
            return
        self.pause_animation()
        if not self.path:
            return

        new_index = int(float(value))
        if 0 <= new_index < len(self.path):
            self.current_path_index = new_index
            self.draw_current_step()


if __name__ == "__main__":
    root = tk.Tk()
    game = ChessGUI(root)

    # 计算方形窗口，初始比例 1:1，并居中
    side_length = max(game.width + 20, game.height + 200)  # 200 预留底部控件高度
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    x = (screen_width - side_length) // 2
    y = (screen_height - side_length) // 2
    root.geometry(f"{side_length}x{side_length}+{x}+{y}")

    root.mainloop()
