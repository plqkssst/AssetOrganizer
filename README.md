# Asset Organizer for UE4.27

UE4.27 资源整理插件 - 自动按类型组织项目资源的高级工具

## 功能特性

- **62+ 资源类型支持** - 涵盖网格体、材质、纹理、动画、蓝图、音频、VFX等
- **智能依赖排序** - 基于资源依赖关系拓扑排序，确保安全的移动顺序
- **延迟文件夹创建** - 只创建实际需要的文件夹，保持项目结构整洁
- **异步执行** - 不阻塞编辑器UI的操作流程
- **历史记录/撤销** - 完整的历史记录和一键回滚功能
- **引用修复** - 自动修复资源引用关系
- **预览模式** - 在实际执行前预览所有变更
- **根文件夹包装** - 可选将所有整理后的资源放入统一根文件夹

## 支持的资源类型

| 类别 | 类型数量 | 具体类型 |
|------|---------|---------|
| Meshes | 5 | StaticMesh, SkeletalMesh, Skeleton, PhysicsAsset, DestructibleMesh |
| Materials | 6 | Material, MaterialInstance, MaterialFunction, ParameterCollection等 |
| Textures | 7 | Texture2D, TextureCube, RenderTarget, MediaTexture等 |
| Animation | 11 | AnimSequence, AnimMontage, BlendSpace, AimOffset等 |
| Blueprints | 4 | Blueprint, WidgetBlueprint, UserDefinedEnum, UserDefinedStruct |
| AI | 3 | BehaviorTree, BlackboardData, EnvQuery |
| Data | 8 | DataTable, DataAsset, CurveFloat, CurveTable等 |
| Audio | 8 | SoundWave, SoundCue, SoundAttenuation, ReverbEffect等 |
| VFX | 4 | ParticleSystem, NiagaraSystem, NiagaraEmitter等 |
| Environment | 3 | FoliageType, LandscapeGrassType, LandscapeLayerInfo |
| Media | 2 | MediaPlayer, FileMediaSource |
| Levels | 1 | World (Maps) |

## 安装方法

1. 下载本插件
2. 将 `AssetOrganizer` 文件夹复制到你的项目 `Plugins/` 目录下
3. 在编辑器中启用插件：编辑(Edit) > 插件(Plugins) > 项目(Project) > Asset Organizer
4. 重启编辑器

## 使用方法

### 打开插件面板
- 工具栏按钮：在关卡编辑器工具栏中找到"资产整理"按钮
- 菜单路径：窗口(Window) > 资产整理 v2.0

### 主要功能

#### 一键整理
点击"一键整理"按钮，插件将：
1. 扫描所有选中类型的资源
2. 按依赖关系排序
3. 移动资源到目标文件夹
4. 修复所有引用关系
5. 清理空文件夹
6. 保存历史记录

#### 预览模式
点击"预览"按钮可以：
- 查看所有将被移动的资源
- 查看新建的文件夹
- 查看命名冲突
- 确认无误后再执行整理

#### 历史/撤销
- 查看所有历史操作记录
- 一键回滚到整理前的状态
- 自动修复回滚后的引用关系
- 清理回滚产生的空文件夹

#### 设置主文件夹
- 启用后可指定统一根文件夹（如 "Organized"）
- 所有资源将被放入 `/Game/Organized/` 下对应子文件夹

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

## 技术特性

### 引用修复机制
- 使用 UE4 的 `ObjectRedirector` 系统自动处理引用
- 每次批量移动后立即修复重定向器
- 保存关卡以持久化引用修复

### 回滚机制
- 保存操作前的关卡引用快照
- 回滚后自动删除陈旧的 ObjectRedirector
- 自动恢复关卡的资源引用

### 并发控制
- 防止重复刷新操作
- 线程安全的资源计数
- AssetRegistry 委托的正确处理

## 版本历史

### v2.0 (当前版本)
- 修复回滚后资源引用丢失问题
- 修复刷新按钮导致的崩溃问题
- 优化关卡引用快照和恢复机制
- 改进空文件夹清理逻辑

## 作者

- **Created by**: Asking_PLQ
- **Version**: 2.0
- **Engine Version**: 4.27.0

## 许可证

Copyright Asking_PLQ. All Rights Reserved.

## 致谢

- [Allar's UE5 Style Guide](https://github.com/Allar/ue5-style-guide) - 命名规范参考
- Epic Games 官方文档
