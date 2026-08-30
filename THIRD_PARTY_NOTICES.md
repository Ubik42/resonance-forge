# 第三方代码与算法说明

## Synthesis ToolKit in C++（STK）

- 上游仓库：https://github.com/thestk/stk
- 本次参考提交：`6aacd357d76250bb7da2b1ddf675651828784bbc`
- 作者：Perry R. Cook、Gary P. Scavone
- 参考范围：`Plucked` 的 Karplus–Strong 数字波导弦结构，包括噪声激励、延迟线、环路低通与衰减反馈。
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
