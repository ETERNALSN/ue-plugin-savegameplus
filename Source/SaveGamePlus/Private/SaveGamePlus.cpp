#include "SaveGamePlus.h"
#include "SaveGamePlusStatics.h"

DEFINE_LOG_CATEGORY(LogSaveGamePlus);

#define LOCTEXT_NAMESPACE "FSaveGamePlusModule"

void FSaveGamePlusModule::StartupModule()
{
	FSaveGameCustomHeaderFactory::RegisterHeaderType(
		FName("Example"),
		FSaveGameCustomHeaderFactory::FCreateHeaderDelegate::CreateLambda([]
		{
			return MakeShared<FExampleHeader>();
		}));
}

void FSaveGamePlusModule::ShutdownModule()
{
	FSaveGameCustomHeaderFactory::UnregisterHeaderType(FName("Example"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSaveGamePlusModule, SaveGamePlus)
