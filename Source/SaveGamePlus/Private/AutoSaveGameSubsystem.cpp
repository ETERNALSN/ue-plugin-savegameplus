// 版权归ETERNAL星九所有.


#include "AutoSaveGameSubsystem.h"

#include "Interface/SavableObject.h"
#include "SaveGamePlus.h"
#include "SaveGamePlusSettings.h"
#include "Kismet/GameplayStatics.h"

void UAutoSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (const USaveGamePlusSettings* Settings = GetDefault<USaveGamePlusSettings>())
	{
		AutoSaveTime = Settings->AutoSaveGameTime;
	}
	
	RegisterAutoSaveTimer();
	//仅供测试
	//实际时机为GameStart,但现在还未设置
	//GetWorld()->GetTimerManager().UnPauseTimer(AutoSaveTimerHandle);
}

void UAutoSaveGameSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UAutoSaveGameSubsystem::RegisterAutoSaveTimer()
{
	if (!GetWorld())
	{
		UE_LOGFMT(LogSaveGamePlus,Error,"{ClassName}::RegisterAutoSaveTimer -> Invalid world",GetClass()->GetName());
		return;
	}
	if (GetWorld()->GetTimerManager().TimerExists(AutoSaveTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
	}
	
	GetWorld()->GetTimerManager().SetTimer(
		AutoSaveTimerHandle,
		FTimerDelegate::CreateLambda([this]
		{
			if (AutoSaveGameDelegate.IsBound())
			{
				AutoSaveGameDelegate.Broadcast();
			}
#if WITH_EDITOR
			//调试日志
			GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Green,TEXT("Trigger AutoSave"));
#endif
		}),
		AutoSaveTime,
		true);
	GetWorld()->GetTimerManager().PauseTimer(AutoSaveTimerHandle);
	
	AutoSaveGameDelegate.AddUObject(this,&UAutoSaveGameSubsystem::CollectSavedActors);
}

void UAutoSaveGameSubsystem::UnregisterAutoSaveTimer()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
	}
}

void UAutoSaveGameSubsystem::FindSavedActorsByType(const FName& Type, TArray<AActor*>& SavedActors)
{
	if (!AutoSavedActorsByType.IsEmpty() && !Type.IsNone())
	{
		SavedActors = *AutoSavedActorsByType.Find(Type);
	}
}

void UAutoSaveGameSubsystem::CollectSavedActors()
{
	if (!GetWorld() && (GetWorld()->WorldType != EWorldType::Game || GetWorld()->WorldType != EWorldType::PIE))
	{
		return;
	}
	//清除上次保存的信息
	if(!AutoSavedActorsByType.IsEmpty())
	{
		AutoSavedActorsByType.Empty();
	}
	//查找场景中实现SavableObject接口的Actor
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(),USavableObject::StaticClass(),FoundActors);
	
	if (FoundActors.IsEmpty())
	{
		return;
	}

	for (auto AutoSavedActor : FoundActors)
	{
		if (auto It = Cast<ISavableObject>(AutoSavedActor))
		{
			if (!It->GetSaveType().IsNone())
			{
				AutoSavedActorsByType.FindOrAdd(It->GetSaveType()).Add(AutoSavedActor);
			}
		}
	}
}

bool UAutoSaveGameSubsystem::RequestSpawnSavedDynamicActor(AActor* SavedActor)
{
	return true;
}
