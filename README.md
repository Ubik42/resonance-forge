<div align="center">

# 共振铸造台 Resonance Forge

**面向 Unreal 音频开发的物理声源工作台**<br>
把场景碰撞、模态共振、数字波导弦、MIDI 表演和 Wwise 参数发布连接成一条可试听的制作流程。

![Unreal Engine 5.8](https://img.shields.io/badge/Unreal_Engine-5.8.1-0E1128?logo=unrealengine&logoColor=white)
![Wwise 2025.1](https://img.shields.io/badge/Wwise-2025.1-00549F)
![Platform](https://img.shields.io/badge/Platform-Windows_Editor-0078D4?logo=windows&logoColor=white)
![Version](https://img.shields.io/badge/Version-0.3.0-16B8C4)
![License](https://img.shields.io/badge/License-MIT-2EA44F)

</div>

![共振铸造台声学工坊](docs/images/resonance-forge-workshop.png)

> 这不是一个“替你生成音效”的黑盒按钮，而是一套可检查的声源原型：选择场景对象，施加碰撞或 MIDI 激励，决定共振模型，再把同一组参数同步交给 UE Synth 与 Wwise。

## 项目亮点

- **两类实时物理声源**：模态撞击体与八复音数字波导弦。
- **真实场景输入**：碰撞冲量、相对速度和物体尺寸直接影响声音。
- **UE × Wwise 双路径**：UE 原生合成可独立试听，同时发布 Wwise Event 与 3 个 RTPC。
- **可演奏**：MIDI Note On 控制音高与力度，CC1 控制音色明亮度。
- **编辑器工作流**：中文 Slate 面板按“对象 → 激励 → 共振 → 发布”组织操作。
- **可重复验证**：测试音频、PBR 贴图、演示地图和复检报告均可由脚本重建。

## 它解决什么问题

传统游戏音效通常从录音文件开始，材质、碰撞强度和物体尺寸只能通过大量样本与分层规则近似。Resonance Forge 用轻量实时模型把这些信息保留到运行时，让音频设计师和技术音频开发者可以：

1. 在 UE 场景里直接选择一个可发声对象；
2. 用物理碰撞或 MIDI 作为激励；
3. 在模态共振与数字波导之间选择合适模型；
4. 调整能量、明亮度和尺度并立即试听；
5. 将相同参数发送给 Wwise，继续进行混音、路由和发布。

```mermaid
flowchart LR
    A[场景对象] --> B{激励方式}
    B -->|物理碰撞| C[冲量 / 相对速度 / 尺寸]
    B -->|MIDI| D[音高 / 力度 / CC1]
    C --> E{共振模型}
    D --> E
    E -->|模态撞击体| F[离散共振峰]
    E -->|数字波导弦| G[延迟线传播与阻尼反馈]
    F --> H[UE Synth]
    G --> H
    H --> I[Wwise Event + RTPC]
```

## 实机界面

![UE 场景与插件联动](docs/images/resonance-forge-overview.png)

上图记录了 UE 场景、插件面板与 Wwise 参数链路的真实联调状态。当前 `0.3.0` 代码已进一步改成任务导向的声音链布局，并增加“读取当前选择”、空状态和模型相关试听按钮；后续会用新版界面截图替换这张联调图。

## 两种声学模型

| 模型 | 核心结构 | 适用对象 | 可控参数 |
| --- | --- | --- | --- |
| 模态撞击体 | 多组频率、增益与衰减时间不同的共振模态 | 金属板、木块、玻璃、机械结构 | 激励能量、频谱明亮度、共振尺度 |
| 数字波导弦 | 噪声激励、延迟线传播、环路低通、衰减反馈 | 弦、金属丝、可演奏机关 | MIDI 音高、力度、阻尼、反馈与材质耦合 |

数字波导结构参考 STK `Plucked` 的 Karplus–Strong 思路，但没有直接嵌入 STK 整库；核心代码针对 Unreal 音频线程和固定复音池重新实现。来源和许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 最快上手

### 环境要求

- Unreal Engine `5.8.1`
- Windows Editor / Win64
- Wwise Authoring 与 SDK `2025.1.10`
- Wwise Unreal Integration `2025.1.10.9233.4458`

### 安装

1. 克隆仓库：

   ```powershell
   git clone https://github.com/Ubik42/resonance-forge.git
   cd resonance-forge
   ```

2. 使用 Audiokinetic Launcher 将 Wwise Integration 安装到 `Demo/Plugins`。仓库不会重新分发 Wwise SDK 或官方插件。
3. 同步当前插件源码并打开工程：

   ```powershell
   ./scripts/sync_demo_plugin.ps1
   Start-Process ./Demo/ResonanceForgeDemo.uproject
   ```

4. 在 Unreal Editor 中打开：

   ```text
   窗口 → 音频工具 → 共振铸造台 · 材质声源工作台
   ```

### 最短成功路径

1. 点击“打开试听场景”。
2. 在场景中选择钢、木、玻璃砧座或数字波导弦。
3. 点击“读取当前选择”。
4. 选择共振模型和材质预设。
5. 调整激励能量、明亮度与共振尺度。
6. 点击“敲击当前对象”或“拨动当前弦”。
7. 进入 PIE，观察落球碰撞并在 Wwise Profiler 中检查 RTPC。

## 演示场景

### 仓库自带：基础声学工坊

```text
/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab
```

左侧为钢、木、玻璃三组物理碰撞砧座，右侧为数字波导弦，后墙集中显示 Wwise Event 与 RTPC 输出。点击 PIE 后，三颗 `18 kg` 落球会从不同高度下落。

### 本机可选：Fab 增强声学工坊

安装 **Carpenter's Workshop Environment** 后运行：

```powershell
./scripts/regenerate_physics_lab.ps1
```

脚本会在本机生成：

```text
/Game/CarpentersWorkshop/ResonanceForge/L_RF_WorkshopShowcase
```

增强地图加入真实木工台、锤、木槌、木板、钢板和工具箱。Fab 源资产与派生地图均被 Git 忽略；插件检测到增强地图时优先打开，否则自动回退基础场景。

## Wwise 映射

| 类型 | 名称 | 数据来源 |
| --- | --- | --- |
| Event | `Play_RF_Impact_Metal` | 碰撞、MIDI 或面板试听 |
| RTPC | `RF_ImpactEnergy` | 碰撞冲量 / MIDI Velocity |
| RTPC | `RF_ImpactBrightness` | 相对速度 / MIDI CC1 |
| RTPC | `RF_ObjectSize` | 场景对象共振尺度 |

## 工程结构

```text
ResonanceForge
├── Source/ResonanceForgeRuntime   # 模态合成、数字波导与复音池
├── Source/ResonanceForgeWwise     # 碰撞 Actor、MIDI 与 Wwise 桥接
├── Source/ResonanceForgeEditor    # 中文 Slate 声学工作台
├── Demo/Demo_WwiseProject         # 独立 Wwise Authoring 工程
├── Demo/TestAudio/Generated       # 确定性脚本合成撞击 WAV
├── Demo/TestMaterials/Generated   # 自生成钢、木、玻璃 PBR 贴图
├── Demo/Scripts                   # 场景生成、截图与后台复检
├── docs                           # 使用、素材、截图与集成说明
└── scripts                        # 同步、素材生成与一键重建
```

## 可重复重建

```powershell
./scripts/generate_test_impacts.ps1
./scripts/generate_material_textures.ps1
./scripts/sync_demo_plugin.ps1
./scripts/regenerate_physics_lab.ps1
```

随仓库发布的 WAV 与 PBR 贴图均由确定性脚本生成。地图重建完成后会重新从磁盘加载关卡，验证三个物理落球和一个数字波导弦 Actor，而不是只检查脚本是否返回成功。

## 验证证据

| 检查项 | 当前结果 |
| --- | --- |
| UE 5.8 Editor 编译 | 通过 |
| 物理碰撞映射测试 | 通过 |
| 内置声学预设测试 | 通过 |
| Wwise 生成资源检查 | 通过 |
| Wwise RTPC 映射检查 | 通过 |
| 基础地图磁盘重载 | 3 个落球 + 1 个波导弦通过 |
| Carpenter's Workshop UE 5.8 加载 | 通过，本机可选依赖 |

自动化报告默认输出到 `artifacts/automation-*`，地图与 Fab 复检报告输出到 `Demo/Saved/ResonanceForge`。

## 已知限制

- 当前验证范围是 Windows Editor；尚未把 Cook、Shipping 和其他平台写成已完成能力。
- 波导模型采用经典 Karplus–Strong 结构，不等同于 Pianoteq 一类完整钢琴物理建模系统。
- 尚未模拟琴槌接触、踏板、弦间耦合、音板传播和复杂辐射体。
- Wwise SDK、Unreal Integration 与 Fab 商业素材必须由使用者自行安装。
- 当前仓库中的插件联动截图来自上一轮实机界面；`0.3.0` 新版面板截图将在下一轮人工截取后更新。

## 文档

- [演示与截图指南](docs/演示与截图指南.md)
- [测试素材说明](docs/测试素材说明.md)
- [Wwise 集成记录](docs/Wwise集成记录.md)
- [产品与技术选型](docs/产品与技术选型.md)
- [第三方代码与算法说明](THIRD_PARTY_NOTICES.md)

## 许可证

项目代码采用 [MIT License](LICENSE)。Wwise SDK、Wwise Unreal Integration、Fab 素材和其他第三方内容继续遵循各自许可，本仓库不授予其再分发权。
