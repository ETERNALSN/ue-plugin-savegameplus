#pragma once

#include "CoreMinimal.h"
#include "Interface/SavableObject.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "ExampleSaveActor.generated.h"

struct FSaveGameLoadConfig;
struct FSaveGameLoadResult;
class USaveGame;

UCLASS()
class SAVEGAMEPLUS_API AExampleSaveActor : public AActor,public ISavableObject
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AExampleSaveActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual FName GetSaveType() const override
	{
		return FName("Example");
	}
	virtual FDynamicActorSaveData GetDynamicActorSaveData() const override
	{
		FDynamicActorSaveData SaveData = {};
		return SaveData;
	};
	
	UFUNCTION(BlueprintCallable)
	void Save(USaveGame* SaveGameObject, const FString& SlotName);
	
	UFUNCTION(BlueprintCallable)
	void Load(const FString& SlotName, const FSaveGameLoadConfig& Config, FSaveGameLoadResult& OutResult);
	//收集存档数据
	void CollectSaveGameData();
	//加载存档数据
	void LoadSaveGameData();
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere,SaveGame)
	FString ExampleStr;
	
	/* 存档数据 */
	UPROPERTY(SaveGame) FTransform CurrentTransform;
};

UCLASS()
class UExampleSaveGamePlus : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame,BlueprintReadWrite)
	AExampleSaveActor* ExampleSaveActor;
	
	UPROPERTY(SaveGame)
	TArray<FDynamicActorSaveData> DynamicActors;
};
