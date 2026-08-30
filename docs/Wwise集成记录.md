# Wwise 集成记录

## 已验证版本

- Unreal Engine：5.8.1
- Wwise Unreal Integration：2025.1.10.9233.4458
- Wwise Authoring / SDK：2025.1.10.9233
- 最新版研究环境：Wwise 2026.1.3.9276（与 UE 5.8 官方 Integration 分开安装）
- UE 工程：`Demo/ResonanceForgeDemo.uproject`
- Wwise 工程：`Demo/Demo_WwiseProject/Demo_WwiseProject.wproj`

UE 5.8 Demo 使用 Launcher 当前提供的 2025.1 Integration。2026.1 保留用于研究 Authoring、WAAPI 和新声源能力，不向 2025.1 Integration 强行混用 SDK。

## 2026-08-29 至 2026-08-30 完成内容

1. Launcher 已将 Wwise、WwiseSoundEngine、WwiseNiagara 等模块部署到 `Demo/Plugins/Wwise`。
2. `ResonanceForgeDemoEditor Win64 Development` 完成首次全量编译，共 487 个构建动作，结果成功。
3. 新增独立 `ResonanceForgeWwise` 模块，核心合成与中间件桥接保持分层。
4. `UResonanceForgeWwiseBridgeComponent::TriggerImpact` 完成以下映射：
   - `Energy 0–1` → `RF_ImpactEnergy 0–100`
   - `Brightness 0–1` → `RF_ImpactBrightness 0–100`
   - `ObjectSize 0–1` → `RF_ObjectSize 0–100`
   - 同一次调用可叠加 UE 原生模态合成，并发布 `Play_RF_Impact_Metal`。
5. 自行合成软、中、硬三档金属撞击 WAV，统一放在 `Demo/TestAudio/Generated`。
6. 通过 WAAPI 自动创建 Random Container、Event、三个 Game Parameter 和 `RF_ResonanceForge` SoundBank。
7. Windows SoundBank 生成成功：0 warning、0 error。
8. 修正 `RootOutputPath` 后运行 Wwise Reconcile，成功创建 6 个 UE Wwise 资源和 Init Bank。

## 当前 Wwise 对象

| 类型 | 名称 | 用途 |
| --- | --- | --- |
| Random Container | `RF_Impact_Metal` | 在三档金属撞击素材间随机播放 |
| Event | `Play_RF_Impact_Metal` | UE 撞击触发入口 |
| Game Parameter | `RF_ImpactEnergy` | 撞击强度 |
| Game Parameter | `RF_ImpactBrightness` | 高频明亮度 |
| Game Parameter | `RF_ObjectSize` | 声源尺度 |
| SoundBank | `RF_ResonanceForge` | 演示内容 Bank |

## 可重复构建脚本

- `scripts/generate_test_impacts.ps1`：确定性生成三条 48 kHz / 16-bit / mono 测试 WAV。
- `scripts/provision_wwise_project.ps1`：经 HTTP WAAPI 创建或合并 Wwise 对象、保存工程并验证对象路径。
- `scripts/sync_demo_plugin.ps1`：把仓库根目录的插件源码同步到 Demo。

## 下一阶段

- 为三个 RTPC 配置实际的音量、Pitch、Low-pass 属性曲线，而不只是在 Profiler 中观察参数。
- 创建物理碰撞 Actor 与 MIDI 输入桥，完成“球体撞击 / 键盘演奏 / Wwise Profiler”演示关卡。
- 补录 UE 与 Wwise 双窗口联动截图和作品集演示视频。
