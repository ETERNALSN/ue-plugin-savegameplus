// 版权归ETERNAL星九所有.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SavableObject.generated.h"

/**
 * 
 */
USTRUCT()
struct FDynamicActorSaveData
{
	GENERATED_BODY()
	
	//动态Actor类
	UPROPERTY(SaveGame)
	TSoftClassPtr<AActor> DynamicActorClass;
	//保存时Actor的变换
	UPROPERTY(SaveGame)
	FTransform SavedTransform;
	//动态actor保存的数据
	UPROPERTY(SaveGame)
	TArray<uint8> DynamicActorData;
};

// This class does not need to be modified.
UINTERFACE()
class USavableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * 实现该接口来让场景中需要保存的对象(通常是Actor)能被收集
 */
class SAVEGAMEPLUS_API ISavableObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FName GetSaveType() const = 0;
	virtual FDynamicActorSaveData GetDynamicActorSaveData() const
	{
		return FDynamicActorSaveData{};
	}
};
