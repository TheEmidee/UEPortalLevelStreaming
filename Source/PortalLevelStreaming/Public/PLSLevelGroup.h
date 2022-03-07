#pragma once

#include <CoreMinimal.h>
#include <Engine/DataAsset.h>

#include "PLSLevelGroup.generated.h"

UCLASS()
class PORTALLEVELSTREAMING_API UPLSLevelGroup final : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY( EditDefaultsOnly, BlueprintReadWrite, meta = ( AllowedClasses = "World" ) )
    TArray< FSoftObjectPath > Levels;
};
