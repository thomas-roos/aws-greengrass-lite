#include "shared_buffer.hpp"
#include "conv/json_conv.hpp"
#include <buffer_stream.hpp>
#include <sstream>

namespace data {
    void SharedBuffer::put(int32_t idx, ConstMemoryView bytes) {
        putOrInsert(idx, bytes, false);
    }

    void SharedBuffer::insert(int32_t idx, ConstMemoryView bytes) {
        putOrInsert(idx, bytes, true);
    }

    void SharedBuffer::putOrInsert(int32_t idx, ConstMemoryView bytes, bool insert) {
        std::unique_lock guard{_mutex};
        size_t realIdx;
        if(idx < 0) {
            if(insert) {
                idx++; // for insert, -1 is end, so re-adjust
            }
            realIdx = _buffer.size() + idx;
        } else {
            realIdx = idx;
        }
        if(realIdx > _buffer.size()) {
            throw std::out_of_range("Put index out of range");
        }
        if(bytes.size() == 0) {
            // No copy required, but bounds check above still required
            return;
        }
        size_t dataEndIdx = realIdx + bytes.size();
        size_t priorBufferSize = _buffer.size();
        size_t newBufferSize = priorBufferSize;
        if(insert) {
            newBufferSize += bytes.size(); // always increase buffer
        } else if(newBufferSize < dataEndIdx) {
            newBufferSize = dataEndIdx; // increase on demand
        }
        if(newBufferSize > MAX_BUFFER_SIZE) {
            throw std::out_of_range("Buffer size too large");
        }
        if(priorBufferSize != newBufferSize) {
            // after size check
            _buffer.resize(newBufferSize, 0);
        }
        if(insert) {
            // need to move data to make room for insert
            auto destI = _buffer.rbegin();
            auto srcI = destI;
            auto dEnd = destI;
            std::advance(srcI, newBufferSize - priorBufferSize);
            std::advance(dEnd, priorBufferSize - realIdx);
            for(; destI != dEnd; ++srcI, ++destI) {
                *destI = *srcI;
            }
        }
        auto start = _buffer.begin();
        std::advance(start, realIdx);
        auto end = _buffer.begin();
        std::advance(end, dataEndIdx);
        bytes.copyTo(start, end);
    }

    uint32_t SharedBuffer::size() const {
        return _buffer.size();
    }

    void SharedBuffer::resize(uint32_t newSize) {
        std::unique_lock guard{_mutex};
        if(newSize > MAX_BUFFER_SIZE) {
            throw std::out_of_range("Buffer size too large");
        }
        _buffer.resize(newSize, 0);
    }

    uint32_t SharedBuffer::get(int idx, MemoryView bytes) const {
        std::shared_lock guard{_mutex};
        size_t realIdx;
        if(idx < 0) {
            realIdx = _buffer.size() + idx;
        } else {
            realIdx = idx;
        }
        if(realIdx > _buffer.size()) {
            throw std::out_of_range("Get index out of range");
        }
        size_t dataEndIdx = realIdx + bytes.size();
        if(dataEndIdx > _buffer.size()) {
            dataEndIdx = _buffer.size();
        }
        auto start = _buffer.begin();
        std::advance(start, realIdx);
        auto end = _buffer.begin();
        std::advance(end, dataEndIdx);
        return bytes.copyFrom(start, end);
    }

    std::shared_ptr<ContainerModelBase> SharedBuffer::parseJson() {
        // Prevent modification of buffer during conversion - saves double-buffering
        std::shared_lock guard{_mutex};

        util::MemoryReader memReader(_buffer.data(), _buffer.size());
        util::BufferInStreamBase<util::MemoryReader> istream(memReader);
        conv::JsonReader reader(_context.lock());
        data::StructElement value;
        reader.push(std::make_unique<conv::JsonElementResponder>(reader, value));
        rapidjson::ParseResult result = reader.readStream(istream);
        if(!result) {
            if(result.Code() == rapidjson::ParseErrorCode::kParseErrorDocumentEmpty) {
                return {}; // no JSON
            }
            throw errors::JsonParseError();
        }
        guard.unlock();
        return data::Boxed::box(_context.lock(), value);
    }

} // namespace data
