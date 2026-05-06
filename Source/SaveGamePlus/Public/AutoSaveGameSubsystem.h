// 版权归ETERNAL星九所有.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AutoSaveGameSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FAutoSaveGameDelegate)

/**
 * 
 */
UCLASS()
class SAVEGAMEPLUS_API UAutoSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	//注册新的自动保存计时器,如果已存在,覆盖
	void RegisterAutoSaveTimer();
	void UnregisterAutoSaveTimer();
	FAutoSaveGameDelegate& OnAutoSaveGame()
	{
		return AutoSaveGameDelegate;
	}

	/**
	 * 按Type查找自动保存的Actors
	 * @param Type 一般是Actor相关模块,比如"EternalVU"或"ArcaneOstinato"
	 * @param SavedActors 返回找到的Actor
	 */
	UFUNCTION(BlueprintCallable)
	void FindSavedActorsByType(const FName& Type,TArray<AActor*>& SavedActors);
	
	//请求生成保存的动态actor
	static bool RequestSpawnSavedDynamicActor(AActor* SavedActor);
	//收集场景中实现了ISavableObject的Actor,按Type分类
	void CollectSavedActors();
protected:
	FAutoSaveGameDelegate AutoSaveGameDelegate;
	
	UPROPERTY() float AutoSaveTime = 0.f;
	UPROPERTY() FTimerHandle AutoSaveTimerHandle;
	
	TMap<FName,TArray<AActor*>> AutoSavedActorsByType;
};
