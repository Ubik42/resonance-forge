# 第三方代码与算法说明

## Synthesis ToolKit in C++（STK）

- 上游仓库：https://github.com/thestk/stk
- 本次参考提交：`6aacd357d76250bb7da2b1ddf675651828784bbc`
- 作者：Perry R. Cook、Gary P. Scavone
- 参考范围：`Plucked` 的 Karplus–Strong 数字波导弦结构，以及 `BowTable / Bowed` 的弓速—弦速差与非线性摩擦建模思路。
- 集成方式：没有把 STK 整库加入 Unreal。`ResonanceForgeSynthComponent` 按 UE 音频线程和固定复音池重新实现了这部分结构。

许可原文：

```text
The Synthesis ToolKit in C++ (STK)

Copyright (c) 1995-2023 Perry R. Cook and Gary P. Scavone

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

Any person wishing to distribute modifications to the Software is
asked to send the modifications to the original developer so that they
can be incorporated into the canonical version. This is, however, not
a binding provision of this license.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## 未直接集成的调研项目

Faust `physmodels.lib`、FigBug/Piano、OpenPiano 与 Resonarium 只作为技术调研资料，没有复制到本仓库。后面若引入其代码，必须重新评估 LGPL、GPL 或 AGPL 对发布方式的影响。

## REAPER（可选外部 DAW）

- 产品与厂商：REAPER / Cockos Incorporated
- 官网：https://www.reaper.fm/
- 集成方式：插件只生成 UTF-8 文本 `.rpp`，引用同目录的自生成 WAV；不包含、修改或重新分发 REAPER 可执行文件、SDK、脚本、主题或素材。
- 依赖状态：可选。未安装 REAPER 时仍可生成对照带工程，用户可在有授权的工作站继续打开。

REAPER 是独立商业软件，继续遵循 Cockos 的许可条款；本仓库的 MIT License 不覆盖 REAPER。

## Carpenter's Workshop Environment（本机可选布景）

- 来源：Epic Fab 用户资产库。
- 使用范围：仅在本机 Demo 工程中生成增强声学工坊，用于截图和交互验证。
- 仓库状态：原始模型、材质、贴图及派生增强地图均不进入本仓库。
- 依赖状态：可选。未安装时插件自动打开仓库自带的基础声学工坊。

该素材遵循购买/领取时适用的 Fab 内容许可。本仓库不授予其任何再分发权，也不把它描述为本项目作者制作的资产。
