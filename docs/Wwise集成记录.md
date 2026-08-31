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
9. 在 `RF_Impact_Metal` 上建立三条实际 RTPC 曲线：Energy → Volume、Brightness → Low-pass、ObjectSize → Pitch。
10. 配置脚本支持重复执行：已有 WAV 不再重复导入，相同曲线不重建 GUID；连续两次运行的 Work Unit SHA-256 保持一致。

## 当前 Wwise 对象

| 类型 | 名称 | 用途 |
| --- | --- | --- |
| Random Container | `RF_Impact_Metal` | 在三档金属撞击素材间随机播放 |
| Event | `Play_RF_Impact_Metal` | UE 撞击触发入口 |
| Game Parameter | `RF_ImpactEnergy` | 撞击强度 |
| Game Parameter | `RF_ImpactBrightness` | 高频明亮度 |
| Game Parameter | `RF_ObjectSize` | 声源尺度 |
| SoundBank | `RF_ResonanceForge` | 演示内容 Bank |

## 实际塑形曲线

| 输入 | Wwise 属性 | 控制点 |
| --- | --- | --- |
| `RF_ImpactEnergy` | Voice Volume | `0:-24 dB`、`20:-12 dB`、`55:-4 dB`、`100:0 dB` |
| `RF_ImpactBrightness` | Voice Low-pass | `0:82`、`45:34`、`100:0` |
| `RF_ObjectSize` | Voice Pitch | `0:+420 cent`、`50:0`、`100:-520 cent` |

明亮度曲线采用反向 Low-pass：低明亮度保留较强低通，高明亮度逐渐打开高频。尺度曲线以 `50` 为原始音高，小物体升高、大物体降低。

## 可重复构建脚本

- `scripts/generate_test_impacts.ps1`：确定性生成三条 48 kHz / 16-bit / mono 测试 WAV。
- `scripts/provision_wwise_project.ps1`：经 HTTP WAAPI 创建或合并 Wwise 对象、保存工程并验证对象路径。
- `scripts/sync_demo_plugin.ps1`：把仓库根目录的插件源码同步到 Demo。

不依赖 Wwise 用户偏好时，可以先启动官方无界面 WAAPI 服务：

```powershell
& 'C:\Audiokinetic\Wwise_2025.1.10.9233\Authoring\x64\Release\bin\WwiseConsole.exe' `
  waapi-server '.\Demo\Demo_WwiseProject\Demo_WwiseProject.wproj' `
  --http-port 8090 --wamp-port 0 --no-source-control
```

另一个终端运行 `./scripts/provision_wwise_project.ps1`，保存完成后结束 WAAPI 服务，再执行：

```powershell
& 'C:\Audiokinetic\Wwise_2025.1.10.9233\Authoring\x64\Release\bin\WwiseConsole.exe' `
  generate-soundbank '.\Demo\Demo_WwiseProject\Demo_WwiseProject.wproj' `
  --platform Windows --bank RF_ResonanceForge --no-source-control
```

## 当前边界与后续

- 物理碰撞 Actor、数字波导弦、MIDI 输入桥和 Wwise Event / RTPC 发布已经进入基础演示关卡。
- UE 侧会真实发送三个 RTPC，Wwise 端已建立音量、Pitch 与 Low-pass 曲线；当前尚未加入压缩器、材质 Switch 分层或最终响度校准。
- 作品展示优先保留真实工作台、场景联动与 Wwise Profiler 截图，不要求为这个轻量工具单独录制视频。
- 2026.1 仅作为独立研究环境；Demo 在 Audiokinetic 发布兼容版本前继续锁定 2025.1 Integration。
