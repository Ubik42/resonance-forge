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
   - 同一次调用可叠加 UE 原生合成，并按当前材质发布 Steel、Wood 或 Glass Event。
5. 自行合成钢、木、玻璃各软、中、硬三档撞击 WAV，统一放在 `Demo/TestAudio/Generated`。
6. 通过 WAAPI 自动创建三个 Random Container、三个材质 Event、三个 Game Parameter 和 `RF_ResonanceForge` SoundBank。
7. Windows SoundBank 生成成功：0 warning、0 error。
8. 修正 `RootOutputPath` 后运行 Wwise Reconcile，成功创建 6 个 UE Wwise 资源和 Init Bank。
9. 在三个材质 Container 上分别建立三条实际 RTPC 曲线：Energy → Volume、Brightness → Low-pass、ObjectSize → Pitch。
10. 配置脚本支持重复执行：已有 WAV 不再重复导入，相同曲线不重建 GUID；连续两次运行的 Work Unit SHA-256 保持一致。
11. UE 工作台加入“锤击标尺”与“Wwise 出口刻度”：三档力度可直接触发试听，并按曲线控制点显示近似的 dB、Low-pass 与 cent。

## 当前 Wwise 对象

| 类型 | 名称 | 用途 |
| --- | --- | --- |
| Random Container | `RF_Impact_Steel` | 钢材软、中、硬三档撞击素材 |
| Random Container | `RF_Impact_Wood` | 木材软、中、硬三档撞击素材 |
| Random Container | `RF_Impact_Glass` | 玻璃软、中、硬三档撞击素材 |
| Event | `Play_RF_Impact_Steel` | 拉丝钢撞击出口 |
| Event | `Play_RF_Impact_Wood` | 硬木撞击出口 |
| Event | `Play_RF_Impact_Glass` | 薄玻璃撞击出口 |
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

工作台刻度用于快速判断设计方向，不代替 Wwise 的最终曲线求值或 Profiler。它复用相同控制点做近似插值，因此面板明确使用“约”字样。

## 可重复构建脚本

- `scripts/generate_test_impacts.ps1`：确定性生成九条 48 kHz / 16-bit / mono 测试 WAV，覆盖三种材质与三档力度。
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
- UE 侧会根据材质选择三个独立 Event 之一，并真实发送三个 RTPC；Wwise 端已为每个材质 Container 建立音量、Pitch 与 Low-pass 曲线。这里有意使用直观的独立 Event 路由，没有把 Switch Container 当作展示复杂度；当前尚未加入压缩器与最终响度校准。
- 铸样台会在 WAV 旁生成 `.rfrecipe.json` 声源铭牌，记录建议 Event 和三个 RTPC 输入，便于交接；它不会调用 WAAPI 导入、修改 Wwise Work Unit 或渲染 Wwise 效果链。
- 数字波导的拾音位置属于 UE 原始声源 DSP，并随本地配方、共享资产和声源铭牌保存；当前不会虚构为第四个 Wwise RTPC。若后续需要在 Wwise 中实时自动化，再单独设计参数契约与 Authoring 曲线。
- 弦上起振位置同样在 UE 原始声源中塑造初始谐波，并进入本地配方、A/B 与声源铭牌；当前三个 Wwise RTPC 的契约保持不变。
- 指腹、拨片、锤击三种初始弦形，以及弓擦的持续摩擦、弓压和推弓/回弓都属于 UE 波导源，并随配方与铭牌保存；当前 Wwise 路径仍只接收既有 Event 和三个 RTPC，不把弓向伪装成尚未建立的 RTPC 或 Switch。
- 软触、线性、重手力度凸轮在 UE 触发前把键速映射为 Energy；Wwise 接收的是映射后的 `RF_ImpactEnergy`，因此键床与硬件 MIDI 的输出刻度一致。物理碰撞不经过演奏曲线。
- 数字波导弦目前把硬木 Event 当作 Wwise 参考层，独立 String Event 需要 Wwise Authoring / WwiseConsole 与新 SoundBank 后再落地；当前机器只有 Launcher 与 Integration，因此本版没有制造一个无法播放的占位 Event。
- 工作台“监听闸门”在 `AResonanceForgeImpactInstrumentActor::TriggerInstrument` 统一分流：原声炉只触发 UE Synth，Wwise 出口只发布 Event / RTPC，双路叠听分别触发两条链。面板、试音键床、MIDI、键盘与 PIE 碰撞不会各自维护一套分支。
- 监听模式用于调音与排错，不属于可交付声源配方；离线 WAV 仍只渲染 UE 物理声源，避免把实时 Wwise 总线或双路叠听误写成原始素材。
- 铭牌回炉时只恢复 UE 声源参数；Wwise Event 会根据已校验的材质预设重新推导，不直接信任 JSON 中可能过时的 Event 字符串。
- 作品展示优先保留真实工作台、场景联动与 Wwise Profiler 截图，不要求为这个轻量工具单独录制视频。
- 2026.1 仅作为独立研究环境；Demo 在 Audiokinetic 发布兼容版本前继续锁定 2025.1 Integration。
