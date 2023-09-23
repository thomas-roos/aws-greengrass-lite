#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <string>
#include <functional>
#include <c_api.h>

//
// Sugar around the c-api to make cpp development easier
//
namespace ggapi {

    class Struct;
    class StringOrd;
    class ObjHandle;

    typedef std::function<Struct(ObjHandle,StringOrd,Struct)> topicCallbackLambda;
    typedef std::function<void(ObjHandle,StringOrd,Struct)> lifecycleCallbackLambda;
    typedef Struct (*topicCallback_t)(ObjHandle,StringOrd,Struct);
    typedef void (*lifecycleCallback_t)(ObjHandle,StringOrd,Struct);
    extern uint32_t topicCallbackProxy(uintptr_t callbackContext, uint32_t taskHandle, uint32_t topicOrd, uint32_t dataStruct);
    extern void lifecycleCallbackProxy(uintptr_t callbackContext, uint32_t moduleHandle, uint32_t phaseOrd, uint32_t dataStruct);

    class StringOrd {
    private:
        uint32_t _ord;
    public:
        static uint32_t intern(std::string_view sv) {
            return ::ggapiGetStringOrdinal(sv.data(), sv.length());
        }
        explicit StringOrd(std::string_view sv) : _ord {intern(sv)} {
        }
        explicit StringOrd(uint32_t ord) : _ord{ord} {
        }
        bool operator==(StringOrd other) const {
            return _ord == other._ord;
        }

        [[nodiscard]] uint32_t toOrd() const {
            return _ord;
        }

        [[nodiscard]] std::string toString() const {
            size_t len = ::ggapiGetOrdinalStringLen(_ord);
            auto buf = std::make_unique<char[]>(len+1);
            size_t checkLen = ::ggapiGetOrdinalString(_ord, buf.get(), len + 1);
            return {buf.get(), checkLen};
        }
    };

    class ObjHandle {
    protected:
        uint32_t _handle;
    public:
        explicit ObjHandle(uint32_t handle) : _handle {handle} {
        }
        ObjHandle(const ObjHandle & other) = default;
        ObjHandle & operator=(const ObjHandle & other) = default;
        bool operator==(ObjHandle other) const {
            return _handle == other._handle;
        }
        bool operator!=(ObjHandle other) const {
            return _handle != other._handle;
        }
        explicit operator bool() const {
            return _handle != 0;
        }
        bool operator!() const {
            return _handle == 0;
        }

        [[nodiscard]] uint32_t getHandleId() const {
            return _handle;
        }

        [[nodiscard]] ObjHandle anchor(ObjHandle newParent) const {
            return ObjHandle(::ggapiAnchorHandle(newParent.getHandleId(), _handle));
        }

        [[nodiscard]] static ObjHandle claimThread() {
            return ObjHandle(::ggapiClaimThread());
        }

        static void releaseThread() {
            ::ggapiReleaseThread();
        }

        [[nodiscard]] ObjHandle subscribeToTopic(StringOrd topic, topicCallback_t callback) {
            return ObjHandle(::ggapiSubscribeToTopic(getHandleId(), topic.toOrd(), topicCallbackProxy, reinterpret_cast<uintptr_t>(callback)));
        }

        [[nodiscard]] ObjHandle sendToTopicAsync(StringOrd topic, Struct message, topicCallback_t result, time_t timeout = -1);

        [[nodiscard]] Struct sendToTopic(StringOrd topic, Struct message, time_t timeout = -1);
        [[nodiscard]] Struct waitForTaskCompleted(time_t timeout = -1);
        [[nodiscard]] ObjHandle registerPlugin(StringOrd componentName, lifecycleCallback_t callback);

        void release() const {
            ::ggapiReleaseHandle(_handle);
        }

        [[nodiscard]] Struct createStruct();

        [[nodiscard]] static ObjHandle thisTask() {
            return ObjHandle {::ggapiGetCurrentTask()};
        }

    };

    class Struct : public ObjHandle {
    private:

    public:
        explicit Struct(const ObjHandle & other) : ObjHandle{other} {
        }
        explicit Struct(uint32_t handle) : ObjHandle{handle} {
        }

        static Struct create(ObjHandle parent) {
            return Struct(::ggapiCreateStruct(parent.getHandleId()));
        }

        Struct & put(StringOrd ord, uint32_t v) {
            ::ggapiStructPutInt32(_handle, ord.toOrd(), v);
            return *this;
        }

        Struct & put(std::string_view sv, uint32_t v) {
            put(StringOrd(sv), v);
            return *this;
        }

        Struct & put(StringOrd ord, uint64_t v) {
            ::ggapiStructPutInt32(_handle, ord.toOrd(), v);
            return *this;
        }

        Struct & put(std::string_view sv, uint64_t v) {
            put(StringOrd(sv), v);
            return *this;
        }

        Struct & put(StringOrd ord, float v) {
            ::ggapiStructPutFloat32(_handle, ord.toOrd(), v);
            return *this;
        }

        Struct & put(std::string_view sv, float v) {
            put(StringOrd(sv), v);
            return *this;
        }

        Struct & put(StringOrd ord, double v) {
            ::ggapiStructPutFloat64(_handle, ord.toOrd(), v);
            return *this;
        }

        Struct & put(std::string_view sv, double v) {
            put(StringOrd(sv), v);
            return *this;
        }

        Struct & put(StringOrd ord, std::string_view v) {
            ::ggapiStructPutString(_handle, ord.toOrd(), v.data(), v.length());
            return *this;
        }

        Struct & put(std::string_view sv, std::string_view v) {
            put(StringOrd(sv), v);
            return *this;
        }

        Struct & put(StringOrd ord, Struct nested) {
            ::ggapiStructPutStruct(_handle, ord.toOrd(), nested.getHandleId());
            return *this;
        }

        Struct & put(std::string_view sv, Struct v) {
            put(StringOrd(sv), v);
            return *this;
        }

        [[nodiscard]] bool hasKey(StringOrd ord) const {
            return ::ggapiStructHasKey(_handle, ord.toOrd());
        }

        [[nodiscard]] bool hasKey(std::string_view sv) const {
            return hasKey(StringOrd{sv});
        }

        [[nodiscard]] uint32_t getInt32(StringOrd ord) const {
            return ::ggapiStructGetInt32(_handle, ord.toOrd());
        }

        [[nodiscard]] uint32_t getInt32(std::string_view sv) const {
            return getInt32(StringOrd{sv});
        }

        [[nodiscard]] uint64_t getInt64(StringOrd ord) const {
            return ::ggapiStructGetInt32(_handle, ord.toOrd());
        }

        [[nodiscard]] uint64_t getInt64(std::string_view sv) const {
            return getInt64(StringOrd{sv});
        }

        [[nodiscard]] float getFloat(StringOrd ord) const {
            return ::ggapiStructGetFloat32(_handle, ord.toOrd());
        }

        [[nodiscard]] float getFloat(std::string_view sv) const {
            return getFloat(StringOrd{sv});
        }

        [[nodiscard]] double getDouble(StringOrd ord) const {
            return ::ggapiStructGetFloat64(_handle, ord.toOrd());
        }

        [[nodiscard]] double getDouble(std::string_view sv) const {
            return getDouble(StringOrd{sv});
        }

        [[nodiscard]] std::string getString(StringOrd ord) const {
            size_t len = ::ggapiStructGetStringLen(_handle, ord.toOrd());
            auto buf = std::make_unique<char[]>(len+1);
            size_t checkLen = ::ggapiStructGetString(_handle, ord.toOrd(), buf.get(), len + 1);
            return {buf.get(), checkLen};
        }

        [[nodiscard]] std::string getString(std::string_view sv) const {
            return getString(StringOrd{sv});
        }

        [[nodiscard]] Struct getStruct(StringOrd ord) const {
            return Struct(::ggapiStructGetStruct(_handle, ord.toOrd()));
        }

        [[nodiscard]] Struct getStruct(std::string_view sv) const {
            return getStruct(StringOrd{sv});
        }
    };

    Struct ObjHandle::createStruct() {
        return Struct::create(*this);
    }

    ObjHandle ObjHandle::sendToTopicAsync(StringOrd topic, Struct message, topicCallback_t result, time_t timeout) {
        return ObjHandle(::ggapiSendToTopicAsync(topic.toOrd(), message.getHandleId(), topicCallbackProxy, reinterpret_cast<uintptr_t>(result), timeout));
    }

    Struct ObjHandle::sendToTopic(ggapi::StringOrd topic, Struct message, time_t timeout) {
        return Struct(::ggapiSendToTopic(topic.toOrd(), message.getHandleId(), timeout));
    }

    Struct ObjHandle::waitForTaskCompleted(time_t timeout) {
        return Struct(::ggapiWaitForTaskCompleted(getHandleId(), timeout));
    }

    ObjHandle ObjHandle::registerPlugin(StringOrd componentName, lifecycleCallback_t callback) {
        return Struct(::ggapiRegisterPlugin(getHandleId(), componentName.toOrd(), lifecycleCallbackProxy, reinterpret_cast<uintptr_t>(callback)));
    }

    uint32_t topicCallbackProxy(uintptr_t callbackContext, uint32_t taskHandle, uint32_t topicOrd, uint32_t dataStruct) {
        auto callback = reinterpret_cast<topicCallback_t>(callbackContext);
        return callback(ObjHandle{taskHandle}, StringOrd{topicOrd}, Struct{dataStruct}).getHandleId();
    }

    void lifecycleCallbackProxy(uintptr_t callbackContext, uint32_t moduleHandle, uint32_t phaseOrd, uint32_t dataStruct) {
        auto callback = reinterpret_cast<lifecycleCallback_t>(callbackContext);
        callback(ObjHandle{moduleHandle}, StringOrd{phaseOrd}, Struct{dataStruct});
    }

}
