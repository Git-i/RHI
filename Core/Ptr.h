#pragma once

#include <cstddef>
namespace RHI
{
    template <typename T>
    class Ptr
    {
    public:
        Ptr()
        {
            ptr = nullptr;
        }
        template<typename U>
        Ptr(const Ptr<U>& other)
        {
            ptr = other.ptr;
            SafeHold();
        }
        Ptr(T* other)
        {
            ptr = other;
        }
        template<typename U>
        Ptr(Ptr<U>&& other) noexcept
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }

        ~Ptr()
        {
            SafeRelease();
            ptr = nullptr;
        }
        template<typename U>
        U* retrieve_as()
        {
            return (U*)ptr;
        }
        template<typename U>
        void operator=(const Ptr<U>& other)
        {
            ptr = other.ptr;
            SafeHold();
        }
        template<typename U>
        void operator=(Ptr<U>&& other)
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        void operator=(T* other)
        {
            SafeRelease();
            ptr = other;
        }
        void operator=(std::nullptr_t _ptr)
        {
            SafeRelease();
            ptr = _ptr;
        }
        T* operator->() const
        {
            return ptr;
        }
        T* Get() const
        {
            return ptr;
        }
        T** GetAddressOf()
        {
            return &ptr;
        }
        T** operator&()
        {
            return &ptr;
        }
    private:
        template<class U> friend class Ptr;
        void SafeRelease()
        {
            if(ptr && ptr->GetRefCount())
                ptr->Release();
        }
        void SafeHold()
        {
            if (ptr) ptr->Hold();
        }
        T* ptr;
    };
    template<typename T>
    Ptr<T> make_ptr(T* val)
    {
        val->Hold();
        return Ptr<T>(val);
    }
}