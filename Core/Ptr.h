#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>
namespace RHI
{
    template<typename T> class Weak;
    template <typename T>
    class Ptr
    {
    public:
        Ptr()
        {
            ptr = nullptr;
        }
        template<typename U, typename std::enable_if_t<std::is_convertible_v<U*, T*>>>
        Ptr(const Ptr<U>& other) noexcept
        {
            ptr = other.ptr;
            SafeHold();
        }
        Ptr(T* other) noexcept
        {
            ptr = other;
        }
        template<typename U>
        Ptr(Ptr<U>&& other, typename std::enable_if_t<std::is_convertible_v<T*, U*>>) noexcept
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }

        ~Ptr()
        {
            SafeRelease();
            ptr = nullptr;
        }
        [[nodiscard]] Weak<T> retrieve()
        {
            return Weak<T>{ptr};
        }
        template<typename U>
        [[nodiscard]] Weak<U> retrieve_as_forced()
        {
            return Weak<U>{ptr};
        }
        template<typename U, typename std::enable_if_t<std::is_convertible_v<T*, U*>>>
        [[nodiscard]] Weak<U> retrieve_as()
        {
            return Weak<U>{ptr};
        }

        template<typename U>
        Ptr<U> transform()
        {
            return make_ptr((U*)ptr);
        }

        template<typename U>
        void operator=(const Ptr<U>& other)
        {
            SafeRelease();
            ptr = other.ptr;
            SafeHold();
        }
        template<typename U>
        void operator=(Ptr<U>&& other)
        {
            SafeRelease();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        void operator=(T* other) = delete;
        void operator=(std::nullptr_t _ptr)
        {
            SafeRelease();
            ptr = _ptr;
        }
        T* operator->() const
        {
            return ptr;
        }

        T** operator&()
        {
            return &ptr;
        }
    private:
        template<typename U> friend class Ptr;
        template<typename U> friend class Weak;
        friend class Device;
        friend class GraphicsCommandList;
        T* Get() const
        {
            return ptr;
        }
        T** GetAddressOf()
        {
            return &ptr;
        }
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
    template<typename T>
    class Weak
    {
        T* ptr;
    public:
        template<typename U> friend class Ptr;
        Weak(const Ptr<T>& ptr)
        {
            this->ptr = ptr.Get();
        }
        Weak() = default;
        static Weak Null()
        {
            return Weak{nullptr};
        }
        T* operator->() const
        {
            return ptr;
        }
        template< typename U, typename = std::enable_if_t<std::is_convertible_v<T*, U*>> >
        Weak<U> Transform()
        {
            return Weak<U>{(U*)ptr};
        }
        template<typename U>
        Weak<U> ForceTransform()
        {
            return Weak<U>{(U*)ptr};
        }
        void operator=(const Ptr<T>& ptr)
        {
            this->ptr = ptr.Get();
        }
    };
}
