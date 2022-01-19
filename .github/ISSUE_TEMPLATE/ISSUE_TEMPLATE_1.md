---

name: 兼容性缺陷报告
description: 对已有PyMO生态的兼容性缺陷报告在这里进行
title: "[Compatibility]: "
labels: ["bug", "compatibility"]
assignees:
  - Seng-Jik
body:
  - type: dropdown
    id: device
    attributes:
      label: 设备类型
      options:
        - Windows
        - MacOS
        - Linux
        - Old 3DS
        - Old 3DS LL/XL
        - New 3DS
        - New 3DS LL/XL
        - Old 2DS
        - New 2DS
    validations:
      required: true
        
  - type: dropdown
    id: problem_type
    attributes:
      label: 问题类型
      description: 你遇到了什么类型的问题？
      options:
        - CPyMO报告了错误信息 (Default)
        - 在没有报告错误信息的情况下闪退
        - 图像瑕疵
        - 游戏流程不正确
        - 性能问题
        - 卡死、无响应
        - 文字渲染瑕疵
        - 致命性崩溃（如内存错误、3DS输出了系统崩溃信息）
    validations:
      required: true
      
  - type: input
    id: graphics
    attributes:
      label: 显卡（如果不是3DS的话）
      
  - type: input
    id: game_title
    attributes:
      label: 游戏名称
    validations:
      required: true
      
  - type: dropdown
    id: game_package_edition
    attributes:
      label: 游戏数据包版本
      options:
        - s60v3
        - s60v5
        - Android
    validations:
      required: true  
      
  - type: input
    id: problem_script
    attributes:
      label: 此问题发生的脚本文件（可选）
      
  - type: input
    id: problem_script_linenumber
    attributes:
      label: 脚本文件行号（可选）
---
