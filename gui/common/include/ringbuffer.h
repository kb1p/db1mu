#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <common.h>
#include <memory>

template <typename T>
class RingBuffer
{
    std::unique_ptr<T[]> m_data;
    uint m_capacity = 0u,
         m_head = 0u,
         m_size = 0u;

public:
    enum class Exception
    {
        OVERFLOW,
        UNDERFLOW
    };

    RingBuffer(uint cap = 0u)
    {
        reserve(cap);
    }

    void reserve(uint newCap)
    {
        if (newCap > m_capacity)
        {
            std::unique_ptr<T[]> newData { new T[newCap] };
            if (m_data)
            {
                // Copy the old data, set the new head to the beginning
                // of the new array and restore the m_size modified by
                // dequeueRange().
                const auto size = m_size;
                dequeueRange(newData.get(), m_capacity);
                m_head = 0u;
                m_size = size;
            }
            m_data = std::move(newData);
            m_capacity = newCap;
        }
    }

    uint capacity() const noexcept
    {
        return m_capacity;
    }

    uint size() const noexcept
    {
        return m_size;
    }

    void enqueue(T elem)
    {
        if (m_size == m_capacity)
            throw Exception::OVERFLOW;

        m_data[(m_head + m_size++) % m_capacity] = elem;
    }

    T dequeue()
    {
        if (m_size == 0u)
            throw Exception::UNDERFLOW;

        const auto rv = m_data[m_head];
        m_head = (m_head + 1) % m_capacity;
        m_size--;

        return rv;
    }

    void dequeueRange(T *pDst, uint nElems)
    {
        assert(pDst);
        if (m_size < nElems)
            throw Exception::UNDERFLOW;

        for (uint i = 0; i < nElems; i++)
        {
            *pDst++ = m_data[m_head];
            m_head = (m_head + 1) % m_capacity;
        }
        m_size -= nElems;
    }
};

#endif
