# CPyMO HD

*CPyMO HD*是CPyMO功能增强计划的开发代号，在`hd`分支上开发。  
CPyMO HD将引擎扩展功能划分为四个功能级别，每个级别都基于上个级别进行改动：

- Feature Level 0
  * 这个级别和PyMO完全一致。
- Feature Level 1
  * 这个级别删除了PyMO中部分落后的功能。
  * 该级别的backend部分与Feature Level 0保持一致，不需要增强硬件功能。
  * 该级别可能会占用更多的内存，除此之外，所有Feature Level 0可以运行的平台都应可以运行Feature Level 1.
  * 移除了platform s60v3和s60v5的支持。
- Feature Level 2
  * 这个级别在backend上增强图形功能：
    + 旋转
    + 缩放
    + 加法混合模式
- Feature Level 3
  * 这个级别在backend上增强图形功能以使得CPyMO可以完整访问Open GL ES 2.0的所有功能。

每个CPyMO的编译版本都可以支持不同的功能级别，而游戏可以在`gameconfig.txt`的`platform`字段中指定以下值：

- `cpymohd1` - 最低需要使用CPyMO HD Feature Level 1来执行
- `cpymohd2` - 最低需要使用CPyMO HD Feature Level 2来执行
- `cpymohd3` - 最低需要使用CPyMO HD Feature Level 3来执行

除了Feature Level 0之外，每个Feature Level都可以直接访问更低Feature Level的功能，并且需要插入Feature Level判断代码来可选地使用更高Feature Level的功能。


# Feature Level 1

* 禁用logo1和logo2在启动时的显示
* 支持Lua语言，在pymo脚本中新增`#lua`命令，此行后面的部分会被直接当作lua代码执行

## Actor抽象


Actor是一种在游戏中可以被作为游戏物体执行的对象，它具有以下结构：

```lua

{
    -- 无返回值，用于在需要绘制该对象时进行绘制操作
    -- 不保证每帧都会调用
    -- 该函数不应该修改self的内容
    draw = function(self)
    end,

    -- 无返回值，在当前对象及所有子对象绘制结束之后调用
    late_draw = function(self)
    end,

    -- 无返回值，用于在需要更新该对象时进行更新操作
    -- 保证每帧调用一次
    -- 距离上一帧的时间将会被作为delta_time: float参数传入
    update = function(self, delta_time)
    end,

    -- 无返回值，用于在当前对象及所有子对象更新结束之后调用
    -- 保证每帧调用一次
    -- 距离上一帧的时间将会被作为delta_time: float参数传入
    late_update = function(self, delta_time)
    end,

    -- 在该对象即将被销毁时调用
    -- 需要在这里销毁所有的非GC对象
    free = function(self)
    end,

    -- 该对象及子对象已经被free了之后调用
    -- 用于销毁具有特殊依赖关系的非GC对象
    late_free = function(self)
    end,

    -- 该对象内将会被用于递归地存储子actor列表
    children = {},

    -- 所有的对象均可为nil，CPyMO将会跳过这些处理
    -- 除此之外，还可以放入其他对象用于保存本actor的状态信息
}

```

CPyMO将会从全局的`main`表作为actor进行执行，在进入UI状态时将不会执行`main` actor。


## 工具类型

工具类型是一种Lua表，用于在Lua端和C端交换数据。

### rect

具有以下字段：

* `x: number` 
* `y: number`
* `w: number`
* `h: number`

### color

具有以下字段，所有值范围均在0~255之间：

* `r: number`
* `g: number`
* `b: number`
* `a: number | nil`


## Lua API

所有的CPyMO Lua API都存储在包`cpymo`中，以下为`cpymo`包中的内容：

* `gamedir: string` - 表示游戏所在文件夹
* `feature_level: int` - 当前运行在哪个Feature Level的引擎上
  - 可能会大于`gameconfig.txt`中的值，这种情况下可以访问更高级别的功能
* `vars: userdata` - 这个表包含了所有的PyMO变量，仅可在其中读写整数，不可遍历
* `readonly(table) : userdata` - 创建表的只读句柄，可以通过该句柄读取表的内容，但不能写入
* `is_skipping() : bool` - 检查是否正在跳过
* `extract_text(string)` - 导出游戏文本以供游戏在CUI模式下运行或提供给视障人员，需要多次调用后使用`extract_text_submit`来进行一次导出
* `extract_text_submit()` - 提交一次`extract_text`导出的文本
* `exit()` - 退出游戏

### `cpymo.render`

供Lua绘制的屏幕空间的左上角坐标为`(0, 0)`，屏幕的长度和宽度由`gameconfig.txt`中`imagesizew`和`imagesizeh`字段定义。

这个包用于存储与渲染有关的功能，以下为此包内容：

* `request_redraw()` - 请求绘制新一帧
* `fill_rect(dst: rect, color, semantic)` - 在下一帧绘制一个实心矩形

#### `cpymo.render.semantic`

这个包内包含了一些绘图语义：

* `bg` - 背景
* `chara` - 立绘
* `ui_bg` - UI层背景
* `ui_element_bg` - UI元素的背景
* `ui_element` - UI元素
* `text_say` - 对话文本
* `text_say_textbox` - 对话文本框
* `text_text` - text命令生成的文本
* `titledate_bg` - title_dsp命令与date命令生成的背景
* `titledate_text` - title_dsp命令与date命令生成的文本
* `sel_img` - 选项
* `effect` - 后处理特效

#### 类`cpymo_render_image`

这个类包含以下成员：

* `get_size(self) : number, number` - 获取大小
* `draw(self, dst: rect, src: rect | nil, alpha: number, semantic)` - 绘制此图像到下一帧，其中alpha范围在0~255之间
* `free(self)` - 手动释放其内存

### `cpymo.asset`

* `load_bg(bg_name: string) : cpymo_render_image` - 加载bg图像
* `load_chara(chara_name: string) : cpymo_render_image` - 加载chara图像
* `load_system_image(image_name: string) : cpymo_render_image` - 加载system图像

### `cpymo.ui`

* `enter(ui: actor)` - 进入一层UI
* `exit()` - 退出当前UI
* `msgbox(msg: string)` - 弹出一个消息框
* `okcancelbox(msg: string, callback: bool -> void)` 
  - 弹出“确定/取消”框，当关闭此框时调用`callback`
  - 若选择了“确定”，则`callback`会被传入`true`，其他情况下传入`false`

### `cpymo.flags`

该包内的功能用于管理Flag，每个Flag由一个字符串组成，可检查其存在或不存在。    
这些Flag是全局的，将会在全局存档中保存。    
这里的Flag最终使用一个Hash值表示，不保证可靠。

* `set(flag: string)` - 设置一个flag
* `check(flag: string) : bool` - 检查这个flag是否存在
* `unset(flag: string)` - 取消一个flag
* `unlock_cg(cg_name: string)` - 解锁一个CG
* `lock_cg(cg_name: string)` - 重新锁定一个CG
* `cg_unlocked(cg_name: string) : bool` 检查这个CG是否被锁定


### `cpymo.input`

该包不可使用pairs遍历，表示当前帧引擎所获取的输入。

其中以下值为`bool`类型：

* `up`, `down`, `left`, `right` - 方向键
* `ok` - 确定键
* `cancel` - 取消键/鼠标右键
* `skip` - 跳过键
* `hide_window` - 隐藏对话框键
* `mouse_button` - 鼠标左键/触摸屏幕是否按下
* `mouse_position_useable` - 鼠标指针坐标是否可用/触摸屏坐标是否可用

以下值为`number`类型：

* `mouse_x`, `mouse_y` - 鼠标坐标/触摸坐标，仅在`mouse_position_usable`为`true`时可用
* `mouse_wheel_delta` - 鼠标滚轮在上一瞬间滑动量

### `cpymo.prev_input`

该包内存放了上一帧的输入，与`cpymo.input`内容相同。

