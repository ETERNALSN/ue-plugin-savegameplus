// 版权归ETERNAL星九所有.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SaveGamePlusSettings.generated.h"

/**
 * 
 */
UCLASS(Config="SaveGamePlus",DefaultConfig,meta=(DisplayName="SaveGamePlus"))
class SAVEGAMEPLUS_API USaveGamePlusSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config,EditAnywhere,DisplayName="自动保存周期(秒)",Category="SaveGamePlus")
	float AutoSaveGameTime = 600.f;
};
