#include "SaveGamePlusStatics.h"

#include "SaveGameSerializable.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Async/Async.h"

TMap<FName, FSaveGameCustomHeaderFactory::FCreateHeaderDelegate>& 
	FSaveGameCustomHeaderFactory::GetRegistry()
{
	static TMap<FName, FCreateHeaderDelegate> Registry;
	return Registry;
}

void FSaveGameCustomHeaderFactory::RegisterHeaderType(FName HeaderType, FCreateHeaderDelegate FactoryDelegate)
{
	GetRegistry().Add(HeaderType, FactoryDelegate);
}

void FSaveGameCustomHeaderFactory::UnregisterHeaderType(FName HeaderType)
{
	GetRegistry().Remove(HeaderType);
}

TSharedPtr<ISaveGameCustomHeader> FSaveGameCustomHeaderFactory::CreateHeader(FName HeaderType)
{
	if (FCreateHeaderDelegate* Delegate = GetRegistry().Find(HeaderType))
	{
		return Delegate->Execute();
	}
	return nullptr;
}

TArray<FName> FSaveGameCustomHeaderFactory::GetRegisteredTypes()
{
	TArray<FName> Types;
	GetRegistry().GetKeys(Types);
	return Types;
}

uint32 USaveGamePlusStatics::CalculateCRC32(const TArray<uint8>& InData)
{
	return FCrc::MemCrc32(InData.GetData(), InData.Num());
}

FString USaveGamePlusStatics::GetSaveGamePath(const FString& SlotName)
{
	return FPaths::ProjectSavedDir() / SlotName + TEXT(".sav");
}

bool USaveGamePlusStatics::SaveGameToMemoryEx(
	USaveGame* SaveGameObject,
	TArray<uint8>& OutSaveData,
	TSharedPtr<ISaveGameCustomHeader> CustomHeader,
	bool bCompress)
{
	if (!SaveGameObject)
	{
		return false;
	}
	
	//序列化SaveGame对象到内存
	TArray<uint8> PayloadData;
	FMemoryWriter MemoryWriter(PayloadData, true);
	FSaveGamePlusArchive Archive(MemoryWriter, false);
	FSaveFileHeader StandardHeader;
	
	SaveGameObject->Serialize(Archive);
	//如果实现了存档序列化接口
	if (auto It = Cast<ISaveGameSerializable>(SaveGameObject))
	{
		StandardHeader.bHasCustomSerialization = true;
		It->SerializeSaveGame(Archive);
	}
	StandardHeader.OriginalSize = PayloadData.Num();
	StandardHeader.PayloadCRC = CalculateCRC32(PayloadData);
	
	TArray<uint8> FinalPayloadData;
	if (bCompress && PayloadData.Num() > 1024)	//小于1KB不压缩
	{
		int32 CompressedSize = PayloadData.Num() *4 /3 + 16;	//预留空间
		FinalPayloadData.SetNumUninitialized(CompressedSize);
		
		FOodleDataCompression::ECompressor Compressor = FOodleDataCompression::ECompressor::Kraken;
		FOodleDataCompression::ECompressionLevel Level = FOodleDataCompression::ECompressionLevel::Normal;
		
		if (FOodleDataCompression::Compress(
			FinalPayloadData.GetData(),
			CompressedSize,
			PayloadData.GetData(),
			PayloadData.Num(),
			Compressor,
			Level))
		{
			FinalPayloadData.SetNum(CompressedSize);
			StandardHeader.bIsCompressed = true;
			StandardHeader.CompressedSize = CompressedSize;
		}
		else
		{
			//压缩失败使用原始数据
			FinalPayloadData = PayloadData;
			StandardHeader.CompressedSize = PayloadData.Num();
		}
	}
	else
	{
		FinalPayloadData = PayloadData;
		StandardHeader.CompressedSize = PayloadData.Num();
	}
	
	//序列化自定义文件头
	TArray<uint8> CustomHeaderData;
	FName CustomHeaderType = NAME_None;
	if (CustomHeader.IsValid())
	{
		FMemoryWriter CustomWriter(CustomHeaderData, true);
		CustomHeader->Serialize(CustomWriter);
		CustomHeaderType = CustomHeader->GetHeaderType();
	}
	
	//组合最终文件
	//格式:[BaseHeader][StandardHeader][CustomHeader][FinalPayloadData]
	FMemoryWriter FinalWriter(OutSaveData, true);
	FSaveFileBaseHeader BaseHeader;
	BaseHeader.CustomHeaderSize = CustomHeaderData.Num();
	FinalWriter << BaseHeader;
	FinalWriter << StandardHeader;
	
	if (CustomHeaderData.Num() > 0)
	{
		FinalWriter << CustomHeaderType;	//写入类型标识
		FinalWriter.Serialize(CustomHeaderData.GetData(),CustomHeaderData.Num());
	}
	
	FinalWriter.Serialize(FinalPayloadData.GetData(),FinalPayloadData.Num());
	
	return true;
}

bool USaveGamePlusStatics::SaveGameToSlotEx(
	USaveGame* SaveGameObject,
	const FString& SlotName,
	TSharedPtr<ISaveGameCustomHeader> CustomHeader,
	bool bCompressed)
{
	TArray<uint8> SaveData;
	if (!SaveGameToMemoryEx(SaveGameObject, SaveData,CustomHeader,bCompressed))
	{
		return false;
	}
	return SaveDataToSlot(SaveData,SlotName);
}


bool USaveGamePlusStatics::SaveDataToSlot(const TArray<uint8>& InSaveData, const FString& SlotName)
{
	const FString SavePath = GetSaveGamePath(SlotName);
	const FString Directory = FPaths::GetPath(SavePath);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		if (!PlatformFile.CreateDirectoryTree(*Directory))
		{
			return false;
		}
	}

	return FFileHelper::SaveArrayToFile(InSaveData, *SavePath);
}

bool USaveGamePlusStatics::LoadGameFromMemoryEx(
	const TArray<uint8>& InSaveData,
	const FSaveGameLoadConfig& Config,
	FSaveGameLoadResult& OutResult)
{
	OutResult.RawFileData = InSaveData;
	
	if (InSaveData.Num() < sizeof(FSaveFileBaseHeader))
	{
		OutResult.ErrorMessage = TEXT("File too small to contain valid header");
		return false;
	}
	
	//读取基础文件头
	FMemoryReader BaseReader(InSaveData);
	FSaveFileBaseHeader BaseHeader;
	BaseReader << BaseHeader;
	
	if (!BaseHeader.IsValid())
	{
		OutResult.ErrorMessage = TEXT("Invalid Header Type");
		return false;
	}
	
	//读取标准文件头
	FSaveFileHeader StandardHeader;
	BaseReader << StandardHeader;
	
	//读取自定义文件头
	TSharedPtr<ISaveGameCustomHeader> LoadedCustomHeader;
	if (BaseHeader.CustomHeaderSize > 0)
	{
		FName HeaderType;
		BaseReader << HeaderType;
		
		//检查类型是否匹配
		if (Config.ExpectedCustomHeaderType != NAME_None && Config.ExpectedCustomHeaderType != HeaderType)
		{
			if (!Config.bAllowMismatchedHeaderType)
			{
				OutResult.ErrorMessage = FString::Printf(
					TEXT("Custom header type mismatch: expected %s, got %s"),
					*Config.ExpectedCustomHeaderType.ToString(),
					*HeaderType.ToString()
				);
				return false;
			}
		}
		
		//尝试创建对应的Header实例
		LoadedCustomHeader = FSaveGameCustomHeaderFactory::CreateHeader(HeaderType);
		if (LoadedCustomHeader.IsValid())
		{
			//读取自定义文件头数据
			TArray<uint8> CustomHeaderRawData;
			CustomHeaderRawData.SetNumUninitialized(BaseHeader.CustomHeaderSize);
			BaseReader.Serialize(CustomHeaderRawData.GetData(),CustomHeaderRawData.Num());
			
			FMemoryReader CustomReader(CustomHeaderRawData);
			LoadedCustomHeader->Serialize(CustomReader);
			//验证自定义文件头
			if (!LoadedCustomHeader->Validate())
			{
				OutResult.ErrorMessage = TEXT("Custom header validation failed");
				return false;
			}
			
			//执行外部自定义验证
			if (Config.CustomValidateCallback.IsBound() && !Config.CustomValidateCallback.Execute(LoadedCustomHeader))
			{
				OutResult.ErrorMessage = TEXT("Custom header callback returned false");
				return false;
			}
		}
	}
	
	//读取payload
	int64 PayloadOffset = BaseReader.Tell();
	int64 PayloadSize = InSaveData.Num() - PayloadOffset;
	
	TArray<uint8> PayloadData;
	PayloadData.SetNumUninitialized(PayloadSize);
	BaseReader.Serialize(PayloadData.GetData(),PayloadSize);
	
	//解压数据
	TArray<uint8> UncompressedData;
	bool bUsedFallback = false;
	if (StandardHeader.bIsCompressed)
	{
		int32 UncompressedSize = StandardHeader.OriginalSize;
		UncompressedData.SetNumUninitialized(UncompressedSize);

		if (!FOodleDataCompression::Decompress(
			UncompressedData.GetData(),
			UncompressedSize,
			PayloadData.GetData(),
			PayloadData.Num()))
		{
			if (!Config.bFallbackToUncompressed)
			{
				OutResult.ErrorMessage = TEXT("Decompression failed");
				return false;
			}
			//回退:数据保持压缩态,跳过CRC校验
			UncompressedData = PayloadData;
			bUsedFallback = true;
		}
		else
		{
			UncompressedData.SetNum(UncompressedSize);
		}
	}
	else
	{
		UncompressedData = PayloadData;
	}

	//CRC校验(回退路径跳过,因为CRC基于解压后的原始数据计算)
	if (!Config.bSkipCRCCheck && !bUsedFallback)
	{
		uint32 ComputedCRC = CalculateCRC32(UncompressedData);

		if (ComputedCRC != StandardHeader.PayloadCRC)
		{
			OutResult.ErrorMessage = TEXT("CRC check failed");
			return false;
		}
	}

	//反序列化SaveGame对象
	if (!Config.SaveGameClass)
	{
		OutResult.ErrorMessage = TEXT("SaveGameClass is null, USaveGame is abstract and cannot be instantiated directly");
		return false;
	}

	FMemoryReader ObjectReader(UncompressedData,true);
	FSaveGamePlusArchive Archive(ObjectReader,true);

	OutResult.SaveGameObject = NewObject<USaveGame>(GetTransientPackage(), Config.SaveGameClass);
	OutResult.SaveGameObject->Serialize(Archive);
	if (StandardHeader.bHasCustomSerialization)
	{
		if (auto It = Cast<ISaveGameSerializable>(OutResult.SaveGameObject))
		{
			It->SerializeSaveGame(Archive);
		}
	}
	OutResult.CustomHeader = LoadedCustomHeader;
	OutResult.bSuccess = true;

	return true;
}

bool USaveGamePlusStatics::LoadGameFromSlotEx(
	const FString& SlotName,
	const FSaveGameLoadConfig& Config,
	FSaveGameLoadResult& OutResult)
{
	const FString SavePath = GetSaveGamePath(SlotName);
	TArray<uint8> SaveData;
	if (!FFileHelper::LoadFileToArray(SaveData, *SavePath))
	{
		OutResult.ErrorMessage = FString::Printf(TEXT("Failed to load file: %s"), *SavePath);
		return false;
	}
	return LoadGameFromMemoryEx(SaveData,Config,OutResult);
}

bool USaveGamePlusStatics::HasCustomHeader(const TArray<uint8>& InSaveData)
{
	if (InSaveData.Num() < sizeof(FSaveFileBaseHeader))
	{
		return false;
	}
	
	FMemoryReader Reader(InSaveData);
	FSaveFileBaseHeader BaseHeader;
	Reader << BaseHeader;
	
	return BaseHeader.IsValid() && BaseHeader.CustomHeaderSize > 0;
}

FName USaveGamePlusStatics::GetCustomHeaderType(const TArray<uint8>& InSaveData)
{
	if (!HasCustomHeader(InSaveData))
	{
		return NAME_None;
	}

	FMemoryReader Reader(InSaveData);
	FSaveFileBaseHeader BaseHeader;
	Reader << BaseHeader;

	FSaveFileHeader StandardHeader;
	Reader << StandardHeader;

	FName HeaderType;
	Reader << HeaderType;

	return HeaderType;
}

bool USaveGamePlusStatics::DeleteSlot(const FString& SlotName)
{
	const FString SavePath = GetSaveGamePath(SlotName);
	IFileManager& FileManager = IFileManager::Get();
	if (FileManager.FileExists(*SavePath))
	{
		return FileManager.Delete(*SavePath);
	}
	return false;
}

bool USaveGamePlusStatics::DoesSlotExist(const FString& SlotName)
{
	return IFileManager::Get().FileExists(*GetSaveGamePath(SlotName));
}

TArray<FString> USaveGamePlusStatics::GetAllSlots()
{
	TArray<FString> SlotNames;
	const FString SaveDir = FPaths::ProjectSavedDir();
	const FString SearchPattern = SaveDir / TEXT("*.sav");

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *SearchPattern, true, false);

	for (const FString& FileName : FoundFiles)
	{
		SlotNames.Add(FPaths::GetBaseFilename(FileName));
	}
	return SlotNames;
}

void USaveGamePlusStatics::AsyncSaveGameToSlotEx(
	USaveGame* SaveGameObject,
	const FString& SlotName,
	FOnAsyncSaveCompleted OnCompleted,
	TSharedPtr<ISaveGameCustomHeader> CustomHeader,
	bool bCompressed)
{
	// 先在主线程完成序列化(UObject操作必须在主线程)
	TArray<uint8> SaveData;
	const bool bSerialized = SaveGameToMemoryEx(SaveGameObject, SaveData, CustomHeader, bCompressed);
	const FString SavePath = GetSaveGamePath(SlotName);
	const FString Directory = FPaths::GetPath(SavePath);

	Async(EAsyncExecution::ThreadPool, [SaveData = MoveTemp(SaveData), SavePath, Directory, bSerialized, OnCompleted = MoveTemp(OnCompleted)]()
	{
		bool bSuccess = false;
		if (bSerialized)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			if (!PlatformFile.DirectoryExists(*Directory))
			{
				PlatformFile.CreateDirectoryTree(*Directory);
			}
			bSuccess = FFileHelper::SaveArrayToFile(SaveData, *SavePath);
		}

		// 在主线程回调
		AsyncTask(ENamedThreads::GameThread, [OnCompleted, bSuccess]()
		{
			OnCompleted.ExecuteIfBound(bSuccess);
		});
	});
}

void USaveGamePlusStatics::AsyncLoadGameFromSlotEx(
	const FString& SlotName,
	const FSaveGameLoadConfig& Config,
	FOnAsyncLoadCompleted OnCompleted)
{
	const FString SavePath = GetSaveGamePath(SlotName);

	Async(EAsyncExecution::ThreadPool, [SavePath, Config, OnCompleted = MoveTemp(OnCompleted)]()
	{
		// 在后台线程读取文件并解压
		TArray<uint8> FileData;
		if (!FFileHelper::LoadFileToArray(FileData, *SavePath))
		{
			AsyncTask(ENamedThreads::GameThread, [OnCompleted, SavePath]()
			{
				FSaveGameLoadResult Result;
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Failed to load file: %s"), *SavePath);
				OnCompleted.ExecuteIfBound(Result);
			});
			return;
		}

		// 关键:必须回到主线程反序列化(NewObject必须在主线程)
		AsyncTask(ENamedThreads::GameThread, [FileData = MoveTemp(FileData), Config, OnCompleted]()
		{
			FSaveGameLoadResult Result;
			LoadGameFromMemoryEx(FileData, Config, Result);
			OnCompleted.ExecuteIfBound(Result);
		});
	});
}
