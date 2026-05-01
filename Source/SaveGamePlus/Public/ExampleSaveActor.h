// 版权归ETERNAL星九所有.

#pragma once

#include "CoreMinimal.h"
#include "SaveGameSerializable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "ExampleSaveActor.generated.h"

struct FSaveGameLoadConfig;
struct FSaveGameLoadResult;
class USaveGame;

UCLASS()
class SAVEGAMEPLUS_API AExampleSaveActor : public AActor
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
class UExampleSaveGamePlus : public USaveGame,public ISaveGameSerializable
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame,BlueprintReadWrite)
	AExampleSaveActor* ExampleSaveActor;
	
	virtual void SerializeSaveGame(FArchive& Ar) override;
};
