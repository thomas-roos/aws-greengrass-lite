#include "../data/globals.h"
#include <c_api.h>
#include <util.h>

using namespace data;

uint32_t ggapiGetStringOrdinal(const char * bytes, size_t len) {
    Global & global = Global::self();
    return global.environment.stringTable.getOrCreateOrd(std::string {bytes, len}).asInt();
}

size_t ggapiGetOrdinalString(uint32_t ord, char * bytes, size_t len) {
    Global & global = Global::self();
    Handle ordH = Handle{ord};
    global.environment.stringTable.assertStringHandle(ordH);
    std::string s { global.environment.stringTable.getString(ordH) };
    util::CheckedBuffer checked(bytes, len);
    return checked.copy(s);
}

size_t ggapiGetOrdinalStringLen(uint32_t ord) {
    Global & global = Global::self();
    Handle ordH = Handle{ord};
    global.environment.stringTable.assertStringHandle(ordH);
    std::string s { global.environment.stringTable.getString(ordH) };
    return s.length();
}

uint32_t ggapiCreateStruct(uint32_t anchorHandle) {
    Global & global = Global::self();
    if (anchorHandle == 0) {
        anchorHandle = tasks::Task::getThreadSelf().asInt();
    }
    auto ss {std::make_shared<SharedStruct>(global.environment)};
    auto owner {global.environment.handleTable.getObject<TrackingScope>(Handle{anchorHandle})};
    return owner->anchor(ss.get())->getHandle().asInt();
}

void ggapiStructPutInt32(uint32_t structHandle, uint32_t ord, uint32_t value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {static_cast<uint64_t>(value)};
    ss->put(ordH, newElement);
}

void ggapiStructPutInt64(uint32_t structHandle, uint32_t ord, uint64_t value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutFloat32(uint32_t structHandle, uint32_t ord, float value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutFloat64(uint32_t structHandle, uint32_t ord, double value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutString(uint32_t structHandle, uint32_t ord, const char * bytes, size_t len) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement { std::string(bytes, len)};
    ss->put(ordH, newElement);
}

void ggapiStructPutStruct(uint32_t structHandle, uint32_t ord, uint32_t nestedHandle) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    auto s2 {global.environment.handleTable.getObject<Structish>(Handle{nestedHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {s2};
    ss->put(ordH, newElement);
}

bool ggapiStructHasKey(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return ss->hasKey(ordH);
}

uint32_t ggapiStructGetInt32(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<uint32_t >(ss->get(ordH));
}

uint64_t ggapiStructGetInt64(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<uint64_t >(ss->get(ordH));
}

float ggapiStructGetFloat32(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<float>(ss->get(ordH));
}

double ggapiStructGetFloat64(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<double>(ss->get(ordH));
}

uint32_t ggapiStructGetStruct(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    std::shared_ptr<ObjectAnchor> ss_anchor {global.environment.handleTable.getAnchor(Handle{structHandle})};
    std::shared_ptr<TrackingScope> ss_root {ss_anchor->getOwner()};
    auto ss {ss_anchor->getObject<Structish>()};
    Handle ordH = Handle{ord};
    return ss_root->anchor(ss.get())->getHandle().asInt();
}

size_t ggapiStructGetStringLen(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    std::string s = ss->get(ordH).getString();
    return s.length();
}

size_t ggapiStructGetString(uint32_t structHandle, uint32_t ord, char * buffer, size_t buflen) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    std::string s = ss->get(ordH).getString();
    util::CheckedBuffer checked(buffer, buflen);
    return checked.copy(s);
}

uint32_t ggapiAnchorHandle(uint32_t anchorHandle, uint32_t objectHandle) {
    Global & global = Global::self();
    if (anchorHandle == 0) {
        anchorHandle = tasks::Task::getThreadSelf().asInt();
    }
    auto ss {global.environment.handleTable.getObject<TrackedObject>(Handle{objectHandle})};
    auto owner {global.environment.handleTable.getObject<TrackingScope>(Handle{anchorHandle})};
    return owner->anchor(ss.get())->getHandle().asInt();
}

void ggapiReleaseHandle(uint32_t objectHandle) {
    Global & global = Global::self();
    std::shared_ptr<ObjectAnchor> anchored {global.environment.handleTable.getAnchor(Handle{objectHandle})};
    // releasing a non-existing handle is a no-op as it may have been garbage collected
    if (anchored) {
        anchored->release();
    }
}
