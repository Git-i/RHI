#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <concepts>
namespace RHI
{
    template<typename T> class Weak;
    template <typename T>
    class Ptr 
    {
    public:
        Ptr() noexcept
        {
            ptr = nullptr;
        }
        Ptr(const Ptr& other) noexcept
        {
            ptr = other.ptr;
            SafeHold();
        }
        Ptr(Ptr&& other) noexcept
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        void operator=(const Ptr& other)
        {
            SafeRelease();
            ptr = other.ptr;
            SafeHold();
        }
        void operator=(Ptr&& other)
        {
            SafeRelease();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        template<typename U, typename std::enable_if_t<std::is_convertible_v<U*, T*>>>
        Ptr(const Ptr<U>& other) noexcept
        {
            ptr = other.ptr;
            SafeHold();
        }
        Ptr(std::nullptr_t other) noexcept
        {
            ptr = other;
        }
        explicit Ptr(T* other) noexcept
        {
            ptr = other;
        }
        template<typename U>
        explicit Ptr(Ptr<U>&& other) noexcept requires std::convertible_to<U*, T*>
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        template<typename U>
        explicit Ptr(const Ptr<U>& other) noexcept requires std::convertible_to<U*, T*>
        {
            ptr = other.ptr;
            SafeHold();
        }

        ~Ptr()
        {
            SafeRelease();
            ptr = nullptr;
        }
        [[nodiscard]] Weak<T> retrieve() const
        {
            return Weak<T>{ptr};
        }
        template<typename U>
        [[nodiscard]] Weak<U> retrieve_as_forced() const
        {
            return Weak<U>{(U*)ptr};
        }
        template<typename U>
        [[nodiscard]] Weak<U> retrieve_as() const requires std::convertible_to<T*, U*>
        {
            return Weak<U>{ptr};
        }

        template<typename U>
        Ptr<U> transform() const
        {
            auto val = Ptr<U>((U*)ptr);
            val.SafeHold();
            return val;
        }

        template<typename U>
        void operator=(const Ptr<U>& other) requires std::convertible_to<U*, T*>
        {
            SafeRelease();
            ptr = other.ptr;
            SafeHold();
        }
        template<typename U>
        void operator=(Ptr<U>&& other) requires std::convertible_to<U*, T*>
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
        bool operator==(const Ptr<T>& other) const
        {
            return ptr == other.ptr;
        }
        T* operator->() const
        {
            return ptr;
        }
        [[deprecated("Try not to use this too often")]]
        T* Raw() const
        {
            return ptr;
        }
        bool IsValid() const
        {
            return ptr != nullptr && ptr->GetRefCount();
        }
        bool NotValid() const
        {
            return !IsValid();
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
    private:
        T* ptr;

    public:
        template<typename U> friend class Ptr;
        template<typename U> friend class Weak;
        Weak(const Ptr<T>& ptr)
        {
            this->ptr = ptr.Get();
        }
        Weak() = default;
        explicit Weak(T* val)
        {
            ptr = val;
        }
        Weak(std::nullptr_t)
        {
            ptr = nullptr;
        }
        T* operator->() const
        {
            return ptr;
        }
        template<typename U>
        Weak<U> Transform() requires std::convertible_to<T*, U*>
        {
            return Weak<U>{static_cast<U*>(ptr)};
        }
        template<typename U>
        Weak<U> ForceTransform()
        {
            return Weak<U>{reinterpret_cast<U*>(ptr)};
        }
        void operator=(const Ptr<T>& ptr)
        {
            this->ptr = ptr.Get();
        }
        [[deprecated("Try not to use this too often")]]
        T* Raw() const
        {
            return ptr;
        }
        bool IsValid()
        {
            return ptr != nullptr && ptr->GetRefCount();
        }
        bool NotValid()
        {
            return !IsValid();
        }
    };
}
