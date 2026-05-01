#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "SaveGamePlusStatics.generated.h"

DECLARE_DELEGATE_OneParam(FOnAsyncSaveCompleted, bool /*bSuccess*/);
DECLARE_DELEGATE_OneParam(FOnAsyncLoadCompleted, const struct FSaveGameLoadResult& /*Result*/);

/**
 * 写入文件标识:SAVE
 */
struct FSaveFileBaseHeader
{
	static constexpr uint32 FILE_TYPE = 0x53415645;	//SAVE
	
	FSaveFileBaseHeader()
		: FileType(FILE_TYPE)
		, CustomHeaderSize(0)
	{}
	
	uint32 FileType;	//文件类型标识
	int32 CustomHeaderSize;
	
	friend FArchive& operator<<(FArchive& Ar, FSaveFileBaseHeader& Header)
	{
		Ar << Header.FileType;
		Ar << Header.CustomHeaderSize;
		return Ar;
	}
	
	bool IsValid() const
	{
		return FileType == FILE_TYPE;
	}
};

struct FSaveFileHeader
{
	FSaveFileHeader()
		: FileVersion(0)
		, OriginalSize(0)
		, CompressedSize(0)
		, bIsCompressed(false)
		, PayloadCRC(0)
		, bHasCustomSerialization(false)
	{}
	uint16 FileVersion;
	int64 OriginalSize;
	int64 CompressedSize;
	bool bIsCompressed;
	uint32 PayloadCRC;
	bool bHasCustomSerialization;
	
	friend  FArchive& operator<<(FArchive& Ar, FSaveFileHeader& Header)
	{
		Ar << Header.FileVersion;
		Ar << Header.bIsCompressed;
		Ar << Header.OriginalSize;
		Ar << Header.CompressedSize;
		Ar << Header.PayloadCRC;
		Ar << Header.bHasCustomSerialization;
		return Ar;
	}
};

/**
 * 自定义文件头接口
 */
class ISaveGameCustomHeader
{
public:
	virtual ~ISaveGameCustomHeader() = default;
	
	virtual void Serialize(FArchive& Ar) = 0;
	virtual FName GetHeaderType() const = 0;
	virtual int64 GetSerializedSize() const = 0;
	virtual bool Validate() const {return true;}
	virtual TSharedPtr<ISaveGameCustomHeader> Clone() const = 0;
};

/**
 * 保存游戏自定义头文件工厂
 * 使用示例:在模块启动StartupModule函数中调用FSaveGameCustomHeaderFactory::RegisterHeaderType注册自定义文件头
 */
class FSaveGameCustomHeaderFactory
{
public:
	DECLARE_DELEGATE_RetVal(TSharedPtr<ISaveGameCustomHeader>,FCreateHeaderDelegate);
	
	static void RegisterHeaderType(FName HeaderType,FCreateHeaderDelegate FactoryDelegate);
	
	static void UnregisterHeaderType(FName HeaderType);
	
	static TSharedPtr<ISaveGameCustomHeader> CreateHeader(FName HeaderType);
	
	static TArray<FName> GetRegisteredTypes();
	
private:
	static TMap<FName,FCreateHeaderDelegate>& GetRegistry();
};

USTRUCT(BlueprintType)
struct FSaveGameLoadResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;
	
	UPROPERTY(BlueprintReadOnly)
	FString ErrorMessage;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USaveGame> SaveGameObject;
	
	TSharedPtr<ISaveGameCustomHeader> CustomHeader;
	
	TArray<uint8> RawFileData;
};

DECLARE_DELEGATE_RetVal_OneParam(bool, FSaveGameCustomValidateDelegate, const TSharedPtr<ISaveGameCustomHeader>&);

USTRUCT(BlueprintType)
struct FSaveGameLoadConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName ExpectedCustomHeaderType = NAME_None;

	//如果自定义头类型不匹配,是否仍尝试加载
	UPROPERTY(BlueprintReadWrite)
	bool bAllowMismatchedHeaderType = true;

	//自定义验证回调
	FSaveGameCustomValidateDelegate CustomValidateCallback;
	//如果CRC校验失败,是否仍然加载
	UPROPERTY(BlueprintReadWrite)
	bool bSkipCRCCheck = false;

	//如果解压缩失败,是否尝试按未压缩读取
	UPROPERTY(BlueprintReadWrite)
	bool bFallbackToUncompressed = true;

	//用于创建SaveGame对象的类(不能为空,USaveGame是抽象类)
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<USaveGame> SaveGameClass;
};

struct FSaveGamePlusArchive : public FObjectAndNameAsStringProxyArchive
{
	FSaveGamePlusArchive(FArchive& InInnerArchive,bool bInLoadIfFindFails) 
		: FObjectAndNameAsStringProxyArchive(InInnerArchive,bInLoadIfFindFails)
	{
		ArIsSaveGame = true;
		ArNoDelta = true;
	}
};

/**
 * 游戏存档Plus静态函数库
 */
UCLASS()
class SAVEGAMEPLUS_API USaveGamePlusStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	static bool SaveDataToSlot(const TArray<uint8>& InSaveData, const FString& SlotName);
	static FString GetSaveGamePath(const FString& SlotName);
	
	static bool SaveGameToMemoryEx(
		USaveGame* SaveGameObject,
		TArray<uint8>& OutSaveData,
		TSharedPtr<ISaveGameCustomHeader> CustomHeader = nullptr,
		bool bCompress = true);
	
	static bool SaveGameToSlotEx(
		USaveGame* SaveGameObject,
		const FString& SlotName,
		TSharedPtr<ISaveGameCustomHeader> CustomHeader = nullptr,
		bool bCompressed = true
		);
	
	static bool LoadGameFromMemoryEx(
		const TArray<uint8>& InSaveData,
		const FSaveGameLoadConfig& Config,
		FSaveGameLoadResult& OutResult
		);
			
	static bool LoadGameFromSlotEx(
		const FString& SlotName,
		const FSaveGameLoadConfig& Config,
		FSaveGameLoadResult& OutResult
		);
	
	//===工具函数===
	static bool HasCustomHeader(const TArray<uint8>& InSaveData);
	static FName GetCustomHeaderType(const TArray<uint8>& InSaveData);
	static uint32 CalculateCRC32(const TArray<uint8>& InData);

	//===Slot管理===
	static bool DeleteSlot(const FString& SlotName);
	static bool DoesSlotExist(const FString& SlotName);
	static TArray<FString> GetAllSlots();

	static void AsyncSaveGameToSlotEx(
		USaveGame* SaveGameObject,
		const FString& SlotName,
		FOnAsyncSaveCompleted OnCompleted,
		TSharedPtr<ISaveGameCustomHeader> CustomHeader = nullptr,
		bool bCompressed = true);

	static void AsyncLoadGameFromSlotEx(
		const FString& SlotName,
		const FSaveGameLoadConfig& Config,
		FOnAsyncLoadCompleted OnCompleted);

};
//==============
//自定义Header示例
//==============
struct FExampleHeader : public ISaveGameCustomHeader
{
	//自定义属性
	FString ExampleName;
	int32 ExampleVersion = 0;
	FDateTime ExampleSaveTime;
	int32 ExamplePlayerLevel = 0;
	
	virtual void Serialize(FArchive& Ar) override
	{
		Ar << ExampleName ;
		Ar << ExampleVersion;
		Ar << ExampleSaveTime;
		Ar << ExamplePlayerLevel;
	}
	
	virtual FName GetHeaderType() const override
	{
		return FName("Example");
	}
	
	virtual int64 GetSerializedSize() const override
	{
		//估算大小,实际序列化时会处理
		return sizeof(int32) * 2 + ExampleName.Len() * sizeof(TCHAR); 
	}
	
	virtual bool Validate() const override
	{
		return !ExampleName.IsEmpty() && ExampleVersion > 0;
	}
	
	virtual TSharedPtr<ISaveGameCustomHeader> Clone() const override
	{
		auto NewHeader = MakeShared<FExampleHeader>();
		NewHeader->ExampleName = ExampleName;
		NewHeader->ExampleVersion = ExampleVersion;
		NewHeader->ExampleSaveTime = ExampleSaveTime;
		NewHeader->ExamplePlayerLevel = ExamplePlayerLevel;
		return NewHeader;
	}
};