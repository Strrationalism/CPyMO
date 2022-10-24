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
