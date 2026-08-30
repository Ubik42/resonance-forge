# 共振铸造台 Resonance Forge

共振铸造台是运行在 Unreal Editor 中的物理声学工作台。它让场景对象既可以作为被碰撞的共振体，也可以成为由 MIDI 演奏的数字波导弦；同一组激励参数还能同步驱动 Wwise Event 与 RTPC。

当前版本专注一条短流程：选择场景对象，选择声学模型，调整激励能量与明亮度，然后立即试听。

## 两种声学模型

### 模态撞击体

将钢、木、玻璃表示为一组频率、增益和衰减时间不同的共振模态。碰撞冲量控制激励能量，相对速度控制频谱明亮度，适合金属板、木块、玻璃和机械结构的撞击声音。

### 数字波导弦

使用噪声激励、延迟线传播、环路低通和衰减反馈实时生成弦振动。它支持八复音、MIDI 音高与力度，并将少量能量耦合到当前材质的模态共振中。算法结构参考 STK `Plucked`，工程内实现针对 Unreal 音频线程重写，来源与许可记录见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 现在能做什么

- 两种实时模型：模态撞击体与八复音数字波导弦。
- 三种材质配置：拉丝钢、硬木、薄玻璃，视觉贴图与模态参数同步切换。
- 物理落球实时触发声音；冲量映射能量，相对速度映射明亮度，物体尺寸映射共振尺度。
- UE 原生合成与 Wwise Event 同时触发，既能独立预听，也能进入中间件管线。
- Wwise `Play_RF_Impact_Metal` Event 与 `RF_ImpactEnergy`、`RF_ImpactBrightness`、`RF_ObjectSize` 三个 RTPC 已接通。
- MIDI Note On 力度控制撞击能量，CC1 控制音色明亮度。
- 中文 Slate 工作区直接显示当前对象、声学响应、模型、材质和实时参数。
- 撞击发生时会按材质显示蓝、橙、青三种短促反馈光，便于录屏解释声音来源。

## 最短使用路径

1. 打开 `窗口 → 音频工具 → 共振铸造台 · 材质声源工作台`。
2. 选择场景中的共振体；如果场景为空，点击“打开试听场景”。
3. 在“模态撞击体”和“数字波导弦”之间切换。
4. 选择材质，调整激励能量、明亮度和共振尺度。
5. 点击“立即试听当前模型”，或者进入 PIE 使用物理碰撞和 MIDI 演奏。

## 试听场景

`/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab`

场景被组织成三座独立实验舱。每座实验舱都有专属 PBR 材质、落球、参数说明牌、灯色和信号轨；三条信号轨最终汇入后墙上的 Wwise Event 核心，直接表达这条链路：

```text
物理撞击 → 材质共振 → MIDI 表演映射 → Wwise Event / RTPC
```

点击 PIE 后，三颗 18 kg 落球会从不同高度下落并分别撞击钢、木、玻璃共振体。

## 安装与依赖

1. 安装 Unreal Engine 5.8.1。
2. 通过 Audiokinetic Launcher 为 UE 5.8 安装 Wwise 2025.1.10 Unreal Integration。仓库不会重新分发 Wwise SDK 与官方插件文件。
3. 克隆仓库后，将本机 Wwise Integration 安装到 `Demo/Plugins`，或使用 Audiokinetic Launcher 的“Integrate Wwise into Project”。
4. 运行 `scripts/sync_demo_plugin.ps1`，把当前插件源码同步到 Demo 工程。
5. 打开 `Demo/ResonanceForgeDemo.uproject`；首次启动时按提示编译 C++ 模块。

## 目录说明

- `Source/ResonanceForgeRuntime`：模态共振、数字波导弦与固定复音池。
- `Source/ResonanceForgeWwise`：物理撞击 Actor、MIDI 输入与 Wwise 桥接。
- `Source/ResonanceForgeEditor`：中文插件控制台。
- `Demo/Demo_WwiseProject`：独立管理的 Wwise Authoring 工程。
- `Demo/TestAudio/Generated`：脚本合成的撞击 WAV。
- `Demo/TestMaterials/Generated`：脚本生成的钢、木、玻璃 PBR 测试贴图。
- `Demo/Scripts`：材质资产、演示地图与后台复检脚本。

## 可重复重建

```powershell
./scripts/generate_test_impacts.ps1
./scripts/generate_material_textures.ps1
./scripts/sync_demo_plugin.ps1
./scripts/regenerate_physics_lab.ps1
```

所有展示贴图和 WAV 都是确定性脚本合成素材，不依赖外部版权资源。地图重建脚本会导入贴图、创建材质图、生成演示关卡，并检查三个落球的 Movable、重力、碰撞与物理模拟状态。

## 已验证环境

- Unreal Engine 5.8.1
- Wwise Unreal Integration 2025.1.10.9233.4458
- Wwise Authoring / SDK 2025.1.10.9233
- Windows Editor / Win64
- 4 项自动化测试全部通过，包含数字波导实际音频缓冲、物理映射、Wwise 资产与 RTPC 检查

## 发布边界

- 本仓库发布本人编写的插件源码、Demo 资产、Wwise 工程结构、自生成声音与贴图素材。
- Wwise SDK、Unreal Integration 与官方插件受 Audiokinetic 许可约束，不进入仓库，需由使用者自行安装。
- `Binaries`、`Intermediate`、`Saved`、Derived Data、Wwise Cache 与用户级设置均被忽略。
- 当前已验证 Windows Editor 开发流程；未把 Cook、Shipping 包和其他平台写成已完成能力。
- 当前数字波导采用经典 Karplus–Strong 结构，不模拟完整钢琴的琴槌接触、踏板、弦间耦合与音板传播。

更多资料：

- [演示与截图指南](docs/演示与截图指南.md)
- [测试素材说明](docs/测试素材说明.md)
- [Wwise 集成记录](docs/Wwise集成记录.md)
- [产品与技术选型](docs/产品与技术选型.md)
