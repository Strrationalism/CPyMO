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

rect表示一个矩形，具有以下字段：

* `x: number` 
* `y: number`
* `w: number`
* `h: number`


## Lua API

所有的CPyMO Lua API都存储在包`cpymo`中，以下为`cpymo`包中的内容：

* `gamedir : string` - 表示游戏所在文件夹
* `engine : userdata` - 指向所属的引擎`cpymo_engine`结构体的指针
* `feature_level` - 当前运行在哪个Feature Level的引擎上
  - 可能会大于`gameconfig.txt`中的值，这种情况下可以访问更高级别的功能

### `cpymo.render`

这个包用于存储与渲染有关的功能，以下为此包内容：

* `request_redraw()` - 请求这一帧刷新屏幕。

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

* `get_size() : number, number` - 获取大小
* `draw(dst: rect, src: rect | nil, alpha: number, semantic)` - 绘制此图像到下一帧
* `free()` - 手动释放其内存

### `cpymo.asset`

* `load_bg(bg_name : string) : cpymo_render_image` - 加载背景图像
