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

        void rootsCheck(const ContainerModelBase * target) const override;

    public:
        explicit SharedStruct(Environment & environment) : StructModelBase{environment} {
        }

        void put(StringOrd handle, const StructElement & element) override;
        void put(std::string_view sv, const StructElement & element) override;
        bool hasKey(StringOrd handle) const override;
        StructElement get(StringOrd handle) const override;
        StructElement get(std::string_view sv) const override;
        std::shared_ptr<StructModelBase> copy() const override;
    };

    /**
     * Typical implementation of ListModelBase
     */
    class SharedList : public ListModelBase {
    protected:
        std::vector<StructElement> _elements;
        mutable std::shared_mutex _mutex;

        void rootsCheck(const ContainerModelBase * target) const override;

    public:
        explicit SharedList(Environment & environment) : ListModelBase{environment} {
        }

        void put(int32_t idx, const StructElement & element) override;
        void insert(int32_t idx, const StructElement & element) override;
        uint32_t length() const override;
        StructElement get(int idx) const override;
        std::shared_ptr<ListModelBase> copy() const override;
    };
}

