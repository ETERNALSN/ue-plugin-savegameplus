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

### [可选] 实现 ISaveGameSerializable 自定义序列化

想要对某些对象或复杂属性进行序列化,可以让 SaveGame 子类实现接口
```cpp
class UMySaveGame : public USaveGame, public ISaveGameSerializable
{
    UPROPERTY(SaveGame)
    AMyActor* MyActor

    virtual void SerializeSaveGame(FArchive& Ar) override
    {
        //检查是否为SaveGame
        if (Ar.IsSaveGame())
        {
            //检查当前是保存还是加载
            if (Ar.IsSaving())
            {
                //序列化Actor中被标记为SaveGame的属性
                MyActor->Serialize(Ar);
            }
            
            if (Ar.IsLoading())
            {
                //反序列化
                MyActor->Serialize(Ar);
            }
        }
    }
};
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
