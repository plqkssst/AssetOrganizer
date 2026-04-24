# Asset Organizer v2.0 for UE4.27

UE4.27 资源整理插件 — 自动按类型组织项目资源的高级工具

支持一键整理、预览模式、白名单、自定义规则、版本回溯，以及与 Project Settings 的完整集成。

<img width="1920" height="1032" alt="V2 01" src="https://github.com/user-attachments/assets/313206c1-a2b5-4030-b46b-c231c4e01d16" />

---

## 目录

- [功能特性](#功能特性)
- [安装方法](#安装方法)
- [使用方法](#使用方法)
  - [打开插件面板](#打开插件面板)
  - [一键整理](#一键整理)
  - [预览模式](#预览模式)
  - [白名单](#白名单)
  - [自定义规则](#自定义规则)
  - [历史撤销](#历史撤销)
  - [Project Settings](#project-settings)
- [支持的资源类型](#支持的资源类型)
- [命名规范](#命名规范)
- [技术特性](#技术特性)
- [版本历史](#版本历史)
- [作者](#作者)
- [许可证](#许可证)

---

## 功能特性

- **62+ 资源类型支持** — 涵盖网格体、材质、纹理、动画、蓝图、音频、VFX 等常用类型
- **智能依赖排序** — 基于资源依赖关系拓扑排序，确保安全的移动顺序（纹理 → 材质 → 网格体 → 蓝图）
- **自定义规则** — 支持引擎内置 62 种类型之外的任何资产类，通过下拉列表选择类名即可添加
- **白名单保护** — 指定文件夹内的资产永远不被移动，避免核心资产被误整理
- **Project Settings 集成** — 在 Edit > Project Settings > Plugins > Asset Organizer 中直接管理所有配置
- **双向同步** — 插件面板与 Project Settings 的自定义规则实时双向同步
- **延迟文件夹创建** — 只创建实际需要的文件夹，保持项目结构整洁
- **异步执行** — 不阻塞编辑器 UI 的操作流程
- **历史记录/撤销** — 完整的历史记录和一键回滚功能
- **引用修复** — 自动修复资源引用关系
- **预览模式** — 在实际执行前预览所有变更
- **根文件夹包装** — 可选将所有整理后的资源放入统一根文件夹
- **引用验证** — 整理完成后自动验证所有移动资产的引用完整性

---

## 安装方法

### 方式一：放入项目插件目录

1. 下载本插件
2. 将 `AssetOrganizer` 文件夹复制到你的项目 `Plugins/` 目录下
3. 重启编辑器
4. 插件会自动启用，无需手动开启

### 方式二：引擎插件目录（所有项目可用）

1. 将 `AssetOrganizer` 文件夹复制到 `E:/Epic/Epic Games/UE_4.27/Engine/Plugins/Editor/` 目录
2. 在任何项目中编辑器 > 插件 > 已安装 > Asset Organizer 中启用
3. 重启编辑器

---

## 使用方法

### 打开插件面板

- **工具栏按钮**：在关卡编辑器工具栏中找到 **"资产整理"** 按钮
- **菜单路径**：窗口(Window) > 资产整理 v2.0

### 一键整理

点击 **"一键整理"** 按钮，插件将：

1. 扫描所有启用类型的资源
2. 按依赖关系排序
3. 移动资源到目标文件夹
4. 修复所有引用关系
5. 清理空文件夹
6. 保存历史记录

### 预览模式

点击 **"预览"** 按钮可以：

- 查看所有将被移动的资源
- 查看新建的文件夹
- 查看命名冲突
- 确认无误后再执行整理

### 白名单

白名单用于保护特定文件夹内的资产，使其永远不被移动。

1. 点击插件面板工具栏中的 **"白名单"** 按钮
2. 点击 **"+ Add"** 选择要保护的文件夹
3. 白名单文件夹内的资产在整理时会被完全跳过
4. 整理完成后自动检查并修复白名单资产指向已移动资产的引用

### 自定义规则

自定义规则用于处理引擎内置 62 种类型之外的自定义 C++ 或 Blueprint 资产类。

#### 在插件面板中添加

1. 在插件面板主列表下方找到 **Custom Rules** 区域
2. 点击 **Class** 下拉列表，选择资产类名
   - 下拉列表包含引擎自带的所有资产类型和项目中实际存在的资产类
   - 若该类属于内置 62 种类型，系统会自动填充推荐的 Prefix 和 TargetPath
3. 点击 **+ Add Rule** 按钮添加规则
4. 点击 **Remove** 删除不需要的规则
5. 点击 **Refresh** 从 Project Settings 同步最新规则状态

#### 在 Project Settings 中管理

**Edit > Project Settings > Plugins > Asset Organizer > Custom Rules**

1. 点击已有规则旁的编辑按钮展开字段
2. **Class Name**：下拉列表选择资产类名（选项与插件面板一致）
3. **Prefix**：设置命名前缀（可选，仅展示用）
4. **Target Path**：设置目标文件夹（如 `/Game/Data/MyType`）
5. **Category Name**：设置 UI 类别名称（可选）
6. **Enabled**：勾选/取消以启用或禁用
7. 修改实时自动保存到 `Config/AssetOrganizer.ini`，插件面板自动同步

### 历史/撤销

- 查看所有历史操作记录
- 一键回滚到整理前的状态
- 自动修复回滚后的引用关系
- 清理回滚产生的空文件夹

### Project Settings

所有插件配置都可以在 **Edit > Project Settings > Plugins > Asset Organizer** 中管理：

- **General**：根路径、是否创建文件夹、是否修复引用等
- **Asset Types**：内置 62 种资产类型的启用/禁用和目标路径
- **Custom Rules**：自定义规则管理（支持下拉列表选择类名）
- **Whitelist**：白名单文件夹列表
- **Verification**：整理后自动验证和自动修复引用
- **History**：历史记录最大条目、是否启用等

---

## 支持的资源类型

| 类别 | 类型数量 | 具体类型 |
|------|---------|---------|
| Meshes | 5 | StaticMesh, SkeletalMesh, Skeleton, PhysicsAsset, DestructibleMesh |
| Materials | 7 | Material, MaterialInstanceConstant, MaterialInstanceDynamic, MaterialFunction, MaterialParameterCollection, SubsurfaceProfile, PhysicalMaterial |
| Textures | 7 | Texture2D, TextureCube, TextureRenderTarget2D, TextureRenderTargetCube, MediaTexture, TextureLightProfile, VolumeTexture |
| Animation | 11 | AnimSequence, AnimMontage, AnimBlueprint, AnimComposite, BlendSpace, BlendSpace1D, AimOffsetBlendSpace, AimOffsetBlendSpace1D, PoseAsset, LevelSequence |
| Blueprints | 4 | Blueprint, WidgetBlueprint, UserDefinedEnum, UserDefinedStruct |
| AI | 3 | BehaviorTree, BlackboardData, EnvQuery |
| Data | 8 | DataTable, DataAsset, StringTable, CurveFloat, CurveVector, CurveLinearColor, CurveTable, CompositeCurveTable |
| Audio | 8 | SoundWave, SoundCue, SoundAttenuation, SoundClass, SoundMix, SoundConcurrency, ReverbEffect |
| VFX | 4 | ParticleSystem, NiagaraSystem, NiagaraEmitter, NiagaraParameterCollection |
| Environment | 3 | FoliageType, LandscapeGrassType, LandscapeLayerInfoObject |
| Media | 2 | MediaPlayer, FileMediaSource |
| Levels | 1 | World (Maps) |

---

## 命名规范

插件基于 [Allar UE5 Style Guide](https://github.com/Allar/ue5-style-guide) 和官方文档：

| 资源类型 | 前缀 | 示例 |
|---------|------|------|
| Static Mesh | SM_ | SM_Rock_01 |
| Skeletal Mesh | SK_ | SK_Character_Player |
| Material | M_ | M_Basic_Wall |
| Material Instance | MI_ | MI_Basic_Wall_Blue |
| Texture | T_ | T_Rock_Normal |
| Blueprint | BP_ | BP_PlayerCharacter |
| Widget Blueprint | WBP_ | WBP_MainMenu |
| Sound Wave | SW_ | SW_Footstep_Stone |
| Sound Cue | SC_ | SC_Footstep_Stone |
| Particle System | PS_ | PS_Fire_Large |
| Niagara System | NS_ | NS_Waterfall |
| Animation | A_ | A_Player_Run |
| Animation Montage | AM_ | AM_Player_Attack |
| Data Table | DT_ | DT_ItemData |
| Curve Float | Curve_ | Curve_DamageFalloff |

---

## 技术特性

### 引用修复机制
- 使用 UE4 的 `ObjectRedirector` 系统自动处理引用
- 每次批量移动后立即修复重定向器
- 保存关卡以持久化引用修复

### 回滚机制
- 保存操作前的关卡引用快照
- 回滚后自动删除陈旧的 ObjectRedirector
- 自动恢复关卡的资源引用

### 引用验证
- 整理完成后自动运行引用验证通道
- 检测所有移动资产是否存在破损引用
- 可选自动尝试通过 redirector fixup 修复破损引用

### 白名单保护
- 白名单文件夹内的资产在整理时被完全跳过
- 整理完成后自动修复白名单资产指向已移动资产的引用

### 自定义规则
- 支持引擎内置 62 种类型之外的任何资产类
- 插件面板和 Project Settings 均支持下拉列表选择类名
- 下拉列表自动收集 AssetTools 注册类型 + AssetRegistry 扫描结果
- 双向同步：插件面板与 Project Settings 实时互联

### 并发控制
- 防止重复刷新操作
- 线程安全的资源计数
- AssetRegistry 委托的正确处理

---

## 版本历史

### v2.0 (当前版本)
- 新增白名单功能：保护指定文件夹内的资产不被移动
- 新增自定义规则功能：支持引擎内置 62 种类型之外的任何资产类
- 新增类选择器下拉列表：插件面板和 Project Settings 均支持下拉选择资产类名
- 新增 Project Settings 集成：`UAssetOrganizerSettings` 改为继承 `UDeveloperSettings`
- 新增双向刷新机制：插件面板与 Project Settings 实时同步
- 新增引用验证和自动修复功能
- 修复回滚后资源引用丢失问题
- 修复刷新按钮导致的崩溃问题
- 修复 `IAssetToolsModule` → `FAssetToolsModule` 编译错误
- 修复 C5038 初始化顺序 Warning
- 优化关卡引用快照和恢复机制
- 优化空文件夹清理逻辑

---

## 作者

- **Created by**: Asking_PLQ
- **Version**: 2.0
- **Engine Version**: 4.27.0

## 许可证

Copyright Asking_PLQ. All Rights Reserved.

## 致谢

- [Allar's UE5 Style Guide](https://github.com/Allar/ue5-style-guide) - 命名规范参考
- Epic Games 官方文档
