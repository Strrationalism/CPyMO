---
name: 兼容性缺陷报告
about: 兼容性缺陷报告
title: "[Compatibility]: "
labels: bug, compatibility
assignees: Seng-Jik

body:
  - type: dropdown
    id: device_and_cpymo_version
    attributes:
      label: CPyMO版本及运行设备
      description: 你在哪个设备上运行哪个版本的CPyMO？
      options:
        - Windows (Default)
        - Old 3DS
        - Old 3DS XL/LL
        - 2DS
        - New 3DS
        - New 3DS XL/LL
        - 2DS XL/LL
        - Linux
        - macOS
    validations:
      required: true
---


