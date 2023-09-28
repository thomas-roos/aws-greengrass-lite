#pragma once
#include "struct_model.h"

namespace data {

    /**
         * Typical implementation of StructModelBase
         */
    class SharedStruct : public StructModelBase {
    protected:
        std::map<StringOrd, StructElement, StringOrd::CompLess> _elements;
        mutable std::shared_mutex _mutex;

        bool putStruct(StringOrd handle, const StructElement & element);

        void rootsCheck(const StructModelBase * target) const override;

    public:
        explicit SharedStruct(Environment & environment) : StructModelBase{environment} {
        }
        std::shared_ptr<SharedStruct> struct_shared_from_this() {
            return std::dynamic_pointer_cast<SharedStruct>(shared_from_this());
        }

        void put(StringOrd handle, const StructElement & element) override;
        void put(std::string_view sv, const StructElement & element) override;
        bool hasKey(StringOrd handle) override;
        StructElement get(StringOrd handle) const override;
        StructElement get(std::string_view sv) const override;
        std::shared_ptr<StructModelBase> copy() const override;
    };
}

