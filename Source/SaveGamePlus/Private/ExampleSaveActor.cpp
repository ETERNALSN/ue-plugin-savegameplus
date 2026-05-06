#include "ExampleSaveActor.h"

#include "AutoSaveGameSubsystem.h"
#include "SaveGamePlusStatics.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AExampleSaveActor::AExampleSaveActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AExampleSaveActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AExampleSaveActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AExampleSaveActor::Save( USaveGame* SaveGameObject, const FString& SlotName)
{
	TSharedPtr<FExampleHeader> Header = MakeShared<FExampleHeader>();
	Header->ExampleName = "Example";
	Header->ExamplePlayerLevel = 1;
	Header->ExampleVersion = 2;
	TArray<AActor*> SavedActors;
	
	if (auto AutoSystem = GetWorld()->GetGameInstance()->GetSubsystem<UAutoSaveGameSubsystem>())
	{
		AutoSystem->FindSavedActorsByType(FName("Example"),SavedActors);
		if (SavedActors.IsEmpty())
		{
			return;
		}
	}
	
	for (auto Actor : SavedActors)
	{
		if (Actor != this)
		{
			if (auto ExampleActor = Cast<AExampleSaveActor>(Actor))
			{
				ExampleActor->CollectSaveGameData();
				if (auto ESGP = Cast<UExampleSaveGamePlus>(SaveGameObject))
				{
					TArray<uint8> OutData;
					FMemoryWriter Writer(OutData,true);
					FSaveGamePlusArchive Archive(Writer,false);
					
					ExampleActor->Serialize(Archive);
					FDynamicActorSaveData SaveData;
					SaveData.DynamicActorClass = ExampleActor->GetClass();
					SaveData.SavedTransform = ExampleActor->GetActorTransform();
					SaveData.DynamicActorData = OutData;
					ESGP->DynamicActors.Add(SaveData);
				}
			}	
		}
	}
	
	USaveGamePlusStatics::SaveGameToSlotEx(SaveGameObject, SlotName, Header);
	
}

void AExampleSaveActor::Load(const FString& SlotName, const FSaveGameLoadConfig& Config, FSaveGameLoadResult& OutResult)
{
	USaveGamePlusStatics::LoadGameFromSlotEx(SlotName, Config, OutResult);
	if (auto s = Cast<UExampleSaveGamePlus>(OutResult.SaveGameObject))
	{
		for (auto& t : s->DynamicActors)
		{
			GetWorld()->SpawnActor(t.DynamicActorClass.LoadSynchronous(),&t.SavedTransform);
		}
	}
}

void AExampleSaveActor::CollectSaveGameData()
{
	CurrentTransform = GetActorTransform();
}

void AExampleSaveActor::LoadSaveGameData()
{
	SetActorTransform(CurrentTransform);
}
