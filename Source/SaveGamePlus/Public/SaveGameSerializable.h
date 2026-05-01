// 版权归ETERNAL星九所有.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveGameSerializable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class USaveGameSerializable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SAVEGAMEPLUS_API ISaveGameSerializable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void SerializeSaveGame(FArchive& Ar) = 0;
};
