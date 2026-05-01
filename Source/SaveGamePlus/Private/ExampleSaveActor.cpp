// 版权归ETERNAL星九所有.


#include "ExampleSaveActor.h"

#include "SaveGamePlusStatics.h"


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
	
	USaveGamePlusStatics::SaveGameToSlotEx(SaveGameObject, SlotName, Header);
	
}

void AExampleSaveActor::Load(const FString& SlotName, const FSaveGameLoadConfig& Config, FSaveGameLoadResult& OutResult)
{
	USaveGamePlusStatics::LoadGameFromSlotEx(SlotName, Config, OutResult);
}

void AExampleSaveActor::CollectSaveGameData()
{
	CurrentTransform = GetActorTransform();
}

void AExampleSaveActor::LoadSaveGameData()
{
	SetActorTransform(CurrentTransform);
}

void UExampleSaveGamePlus::SerializeSaveGame(FArchive& Ar)
{
	if (!Ar.IsSaveGame()) return;
	
	if (Ar.IsSaving())
	{
		if (ExampleSaveActor)
		{
			ExampleSaveActor->CollectSaveGameData();
			ExampleSaveActor->Serialize(Ar);
		}
	}
	if (Ar.IsLoading())
	{
		if (ExampleSaveActor)
		{
			ExampleSaveActor->Serialize(Ar);
			ExampleSaveActor->LoadSaveGameData();
		}
	}
}
