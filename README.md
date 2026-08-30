# 共振铸造台 Resonance Forge

面向 Unreal Engine 客户端音频开发岗位的“物理材质声源工作台”。它把场景中的碰撞冲量、相对速度、物体尺寸与 MIDI 表演数据转换为模态共振，并同步驱动 Wwise Event 与 RTPC。

> 核心表达：**看得见的材质差异，听得见的物理响应，查得到的 Wwise 参数链路。**

![共振铸造台在 Unreal Engine 中的物理材质场景与中文插件界面](docs/images/resonance-forge-overview.png)

## 现在能做什么

- 三种完整材质声源：拉丝钢、硬木、薄玻璃，视觉贴图与声音预设同步切换。
- 物理落球实时触发声音；冲量映射能量，相对速度映射明亮度，物体尺寸映射共振尺度。
- UE 原生模态合成与 Wwise Event 同时触发，既能独立预听，也能进入正式中间件管线。
- Wwise `Play_RF_Impact_Metal` Event 与 `RF_ImpactEnergy`、`RF_ImpactBrightness`、`RF_ObjectSize` 三个 RTPC 已接通。
- MIDI Note On 力度控制撞击能量，CC1 控制音色明亮度。
- 中文 Slate 插件控制台可选择预设、调整三个实时参数、触发撞击并打开演示地图。
- 撞击发生时会按材质显示蓝、橙、青三种短促反馈光，便于录屏解释声音来源。

## 演示场景

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

## 打开插件

在 Unreal Editor 中选择：

```text
窗口 → 音频工具 → 共振铸造台 · 材质声源工作台
```

推荐把工具停靠在右侧：左侧保留演示场景视口，右侧展示材质声源库、参数路由与 Wwise 状态。选择场景中的任意共振体后，点击材质预设会同时修改视觉材质与声音预设。

## 目录说明

- `Source/ResonanceForgeRuntime`：线程安全的 UE 模态合成声源。
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
- 4 项自动化测试全部通过，0 warning，0 failed

## 发布边界

- 本仓库发布本人编写的插件源码、Demo 资产、Wwise 工程结构、自生成声音与贴图素材。
- Wwise SDK、Unreal Integration 与官方插件受 Audiokinetic 许可约束，不进入仓库，需由使用者自行安装。
- `Binaries`、`Intermediate`、`Saved`、Derived Data、Wwise Cache 与用户级设置均被忽略。
- 当前已验证 Windows Editor 开发流程；未把 Cook、Shipping 包和其他平台写成已完成能力。

更多资料：

- [演示与截图指南](docs/演示与截图指南.md)
- [测试素材说明](docs/测试素材说明.md)
- [Wwise 集成记录](docs/Wwise集成记录.md)
- [产品与技术选型](docs/产品与技术选型.md)
