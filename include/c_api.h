#pragma once
#include <cstdint>
#include <cstddef>
#include <ctime>

#if defined(_WIN32)
#define IMPORT __declspec(dllimport)
#define EXPORT __declspec(dllexport)
#else
#define IMPORT
#define EXPORT __attribute__((visibility("default")))
#endif
#if defined(EXPORT_API)
#define IMPEXP EXPORT
#else
#define IMPEXP IMPORT
#endif

typedef uint32_t (*ggapiTopicCallback)(uintptr_t callbackContext, uint32_t taskHandle, uint32_t topicOrd, uint32_t dataStruct);
typedef void (*ggapiLifecycleCallback)(uintptr_t callbackContext, uint32_t moduleHandle, uint32_t phaseOrd, uint32_t dataStruct);

extern "C" void greengrass_lifecycle(uint32_t moduleHandle, uint32_t phase, uint32_t data);

extern "C" IMPEXP uint32_t ggapiGetStringOrdinal(const char * bytes, size_t len);
extern "C" IMPEXP size_t ggapiGetOrdinalString(uint32_t ord, char * bytes, size_t len);
extern "C" IMPEXP size_t ggapiGetOrdinalStringLen(uint32_t ord);
extern "C" IMPEXP uint32_t ggapiClaimThread();
extern "C" IMPEXP void ggapiReleaseThread();
extern "C" IMPEXP uint32_t ggapiCreateStruct(uint32_t anchorHandle);
extern "C" IMPEXP void ggapiStructPutInt32(uint32_t structHandle, uint32_t ord, uint32_t value);
extern "C" IMPEXP void ggapiStructPutInt64(uint32_t structHandle, uint32_t ord, uint64_t value);
extern "C" IMPEXP void ggapiStructPutFloat32(uint32_t structHandle, uint32_t ord, float value);
extern "C" IMPEXP void ggapiStructPutFloat64(uint32_t structHandle, uint32_t ord, double value);
extern "C" IMPEXP void ggapiStructPutString(uint32_t structHandle, uint32_t ord, const char * bytes, size_t len);
extern "C" IMPEXP void ggapiStructPutStruct(uint32_t structHandle, uint32_t ord, uint32_t nestedHandle);
extern "C" IMPEXP bool ggapiStructHasKey(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP uint32_t ggapiStructGetInt32(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP uint64_t ggapiStructGetInt64(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP float ggapiStructGetFloat32(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP double ggapiStructGetFloat64(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP size_t ggapiStructGetStringLen(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP size_t ggapiStructGetString(uint32_t structHandle, uint32_t ord, char * buffer, size_t buflen);
extern "C" IMPEXP uint32_t ggapiStructGetStruct(uint32_t structHandle, uint32_t ord);
extern "C" IMPEXP uint32_t ggapiAnchorHandle(uint32_t anchorHandle, uint32_t objectHandle);
extern "C" IMPEXP void ggapiReleaseHandle(uint32_t objectHandle);
extern "C" IMPEXP uint32_t ggapiGetCurrentTask(void);
extern "C" IMPEXP uint32_t ggapiSubscribeToTopic(uint32_t anchorHandle, uint32_t topicOrd, ggapiTopicCallback rxCallback, uintptr_t callbackContext);
extern "C" IMPEXP uint32_t ggapiSendToTopic(uint32_t topicOrd, uint32_t callStruct, int32_t timeout);
extern "C" IMPEXP uint32_t ggapiSendToTopicAsync(uint32_t topicOrd, uint32_t callStruct, ggapiTopicCallback respCallback, uintptr_t callbackContext, int32_t timeout);
extern "C" IMPEXP uint32_t ggapiCallNext(uint32_t dataStruct);
extern "C" IMPEXP uint32_t ggapiWaitForTaskCompleted(uint32_t asyncTask, int32_t timeout);
extern "C" IMPEXP uint32_t ggapiRegisterPlugin(uint32_t moduleHandle, uint32_t componentName, ggapiLifecycleCallback lifecycleCallback, uintptr_t callbackContext);

// Used only by top-level executable
extern "C" IMPEXP int ggapiMainThread();
