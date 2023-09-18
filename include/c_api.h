#pragma once
#include "../safe_handle.h"

typedef uint32_t (*ggapiTopicCallback)(uint32_t taskHandle, uint32_t topicOrd, uint32_t dataStruct);

extern "C" uint32_t ggapiGetStringOrdinal(const char * bytes, size_t len);
extern "C" size_t ggapiGetOrdinalString(uint32_t ord, char * bytes, size_t len);
extern "C" size_t ggapiGetOrdinalStringLen(uint32_t ord);
extern "C" uint32_t ggapiCreateTask(bool assignToThread);
extern "C" uint32_t ggapiCreateStruct(uint32_t anchorHandle);
extern "C" void ggapiStructPutInt32(uint32_t structHandle, uint32_t ord, uint32_t value);
extern "C" void ggapiStructPutInt64(uint32_t structHandle, uint32_t ord, uint64_t value);
extern "C" void ggapiStructPutFloat32(uint32_t structHandle, uint32_t ord, float value);
extern "C" void ggapiStructPutFloat64(uint32_t structHandle, uint32_t ord, double value);
extern "C" void ggapiStructPutString(uint32_t structHandle, uint32_t ord, const char * bytes, size_t len);
extern "C" void ggapiStructPutStruct(uint32_t structHandle, uint32_t ord, uint32_t nestedHandle);
extern "C" bool ggapiStructHasKey(uint32_t structHandle, uint32_t ord);
extern "C" uint32_t ggapiStructGetInt32(uint32_t structHandle, uint32_t ord);
extern "C" uint64_t ggapiStructGetInt64(uint32_t structHandle, uint32_t ord);
extern "C" float ggapiStructGetFloat32(uint32_t structHandle, uint32_t ord);
extern "C" double ggapiStructGetFloat64(uint32_t structHandle, uint32_t ord);
extern "C" size_t ggapiStructGetStringLen(uint32_t structHandle, uint32_t ord);
extern "C" size_t ggapiStructGetString(uint32_t structHandle, uint32_t ord, char * buffer, size_t buflen);
extern "C" uint32_t ggapiStructGetStruct(uint32_t structHandle, uint32_t ord);
extern "C" uint32_t ggapiAnchorHandle(uint32_t anchorHandle, uint32_t objectHandle);
extern "C" void ggapiReleaseHandle(uint32_t objectHandle);
extern "C" uint32_t ggapiGetCurrentTask(void);
extern "C" uint32_t ggapiSubscribeToTopic(uint32_t anchorHandle, uint32_t topicOrd, ggapiTopicCallback rxCallback);
extern "C" uint32_t ggapiSendToTopic(uint32_t topicOrd, uint32_t callStruct, time_t timeout);
extern "C" uint32_t ggapiSendToTopicAsync(uint32_t topicOrd, uint32_t callStruct, ggapiTopicCallback respCallback, time_t timeout);
extern "C" uint32_t ggapiCallNext(uint32_t dataStruct);
extern "C" uint32_t ggapiWaitForTaskCompleted(uint32_t asyncTask, time_t timeout);
