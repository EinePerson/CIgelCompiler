#pragma once

class ArenaAlocator {
public:
    inline explicit ArenaAlocator(size_t bytes) : m_size(bytes){
        m_buffer = static_cast<std::byte*>(malloc(m_size));
        m_offset = m_buffer;
    }

    template<typename T>
    inline T* alloc(){
        void* offset = m_offset;
        m_offset += sizeof(T);
        return static_cast<T*>(offset);
    }

    inline ArenaAlocator(const ArenaAlocator& other) = delete;

    inline ArenaAlocator operator=(const ArenaAlocator& other) = delete;

    inline ~ArenaAlocator(){
        free(m_buffer);
    }

private:
    size_t m_size;
    std::byte* m_buffer;
    std::byte* m_offset;
};