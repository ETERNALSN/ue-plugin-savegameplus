# SaveGamePlus

增强存档插件，提供压缩、CRC 校验、自定义文件头、自定义序列化和异步操作。

## 文件格式

```
[FSaveFileBaseHeader]  →  魔数(SAVE) + CustomHeader大小
[FSaveFileHeader]      →  版本 + 压缩信息 + CRC + 自定义序列化标记
[CustomHeader]         →  FName类型标识 + 自定义头数据 (可选)
[Payload]              →  UPROPERTY(SaveGame) 数据 + ISaveGameSerializable 自定义数据
```

## 使用示例

### 1. 定义 SaveGame 子类

```cpp
UCLASS()
class UMySaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame)
    int32 PlayerLevel;

    UPROPERTY(SaveGame)
    FString PlayerName;
};
```
### [可选] 实现ISavableObject
实现该接口来让自动保存子系统能够收集关卡中需要保存的Actor
```
UCLASS()
class AMyActor : public AActor, public ISavableObject
{
    GENERATED_BODY()
    
    virtual FName GetSaveType() overrider;
    virtual FDynamicActorSaveData GetDynamicActorSaveData() const override;
};
```
### [可选] 自定义文件头

```cpp
// 定义 Header 类 (实现 ISaveGameCustomHeader)
struct FMyHeader : public ISaveGameCustomHeader { /* ... */ };

// 在模块 StartupModule 中注册
FSaveGameCustomHeaderFactory::RegisterHeaderType(
    FName("MyHeader"),
    FSaveGameCustomHeaderFactory::FCreateHeaderDelegate::CreateLambda([]
    { return MakeShared<FMyHeader>(); }));

// 保存时传入 Header
auto Header = MakeShared<FMyHeader>();
USaveGamePlusStatics::SaveGameToSlotEx(SaveObj, TEXT("Slot1"), Header);

// 加载时校验 Header 类型
Config.ExpectedCustomHeaderType = FName("MyHeader");
```

### 同步保存/加载

```cpp
// 保存
UMySaveGame* SaveObj = NewObject<UMySaveGame>();
SaveObj->PlayerLevel = 10;
USaveGamePlusStatics::SaveGameToSlotEx(SaveObj, TEXT("Slot1"));

// 加载
FSaveGameLoadConfig Config;
Config.SaveGameClass = UMySaveGame::StaticClass();

FSaveGameLoadResult Result;
if (USaveGamePlusStatics::LoadGameFromSlotEx(TEXT("Slot1"), Config, Result))
{
    UMySaveGame* Loaded = Cast<UMySaveGame>(Result.SaveGameObject);
}
```

### 异步保存/加载

```cpp
// 异步保存
USaveGamePlusStatics::AsyncSaveGameToSlotEx(
    SaveObj, TEXT("Slot1"),
    USaveGamePlusStatics::FOnAsyncSaveCompleted::CreateLambda([](bool bSuccess) {}));

// 异步加载
USaveGamePlusStatics::AsyncLoadGameFromSlotEx(
    TEXT("Slot1"), Config,
    USaveGamePlusStatics::FOnAsyncLoadCompleted::CreateLambda([](const FSaveGameLoadResult& Result) {}));
```

###  Slot 管理

```cpp
USaveGamePlusStatics::DoesSlotExist(TEXT("Slot1"));
USaveGamePlusStatics::DeleteSlot(TEXT("Slot1"));
TArray<FString> Slots = USaveGamePlusStatics::GetAllSlots();
```

### 自动保存子系统
新增GameInstanceSubsystem: UAutoSaveSubsystem
设置自动保存时间后,需要手动解除TimerHandle对应Timer的暂停状态,自动保存时会调用委托