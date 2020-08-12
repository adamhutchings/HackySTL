#pragma once

#include "Utility.hpp"

namespace hsd
{
    namespace non_atomic_types
    {
        namespace shared_detail
        {
            template<typename T>
            struct ptr_info
            {
                remove_array_t<T>* _value = nullptr;
                size_t* _size = nullptr;
            };

            template<typename T>
            struct deleter
            {
                template< typename U = T, std::enable_if_t<!std::is_array_v<U>, int> = 0 >
                void operator()(ptr_info<T>& ptr)
                {
                    if(*ptr._size == 1)
                    {
                        delete ptr._value;
                        delete ptr._size;
                        ptr._value = nullptr;
                        ptr._size = nullptr; 
                    }
                    else
                    {
                        ptr._value = nullptr;
                        (*ptr._size)--;
                        ptr._size = nullptr;
                    }
                }

                template< typename U = T, std::enable_if_t<std::is_array_v<U>, int> = 0 >
                void operator()(ptr_info<T>& ptr)
                {
                    if(*ptr._size == 1)
                    {
                        delete[] ptr._value;
                        delete ptr._size;
                        ptr._value = nullptr;
                        ptr._size = nullptr;
                    }
                    else
                    {
                        ptr._value = nullptr;
                        (*ptr._size)--;
                    }
                }
            };
        } // namespace shared_detail

        template< typename T, typename Deleter = shared_detail::deleter<T> >
        class shared_ptr
        {
        private:
            struct _storage : private Deleter
            {
                shared_detail::ptr_info<T> _ptr;

                constexpr Deleter& get_deleter()
                {
                    return *this;
                }
            } _value;

        public:
            using pointer = T*;
            using reference = T&;
            using nullptr_t = decltype(nullptr);
            using remove_array_pointer = remove_array_t<T>*;

            shared_ptr() = default;
            constexpr shared_ptr(nullptr_t) {}

            template< typename U = T, std::enable_if_t<!std::is_array_v<U>, int> = 0 >
            constexpr shared_ptr(pointer ptr)
            {
                _value._ptr._value = ptr;
                _value._ptr._size = new size_t(1);
            }

            template< typename U = T, std::enable_if_t<std::is_array_v<U>, int> = 0 >
            constexpr shared_ptr(remove_array_pointer ptr)
            {
                _value._ptr = ptr;
                _value._ptr._size = new size_t(1);
            }

            constexpr shared_ptr(const shared_ptr& ptr)
            {
                _value = ptr._value;
                (*_value._ptr._size)++;
            }

            constexpr shared_ptr(shared_ptr&& ptr)
            {
                _value = ptr._value;
                ptr._value._ptr._size = nullptr;
                ptr._value._ptr._value = nullptr;
            }

            constexpr ~shared_ptr()
            {
                _value.get_deleter()(_value._ptr);
            }

            constexpr shared_ptr& operator=(nullptr_t)
            {
                _value.get_deleter()(_value._ptr);
                return *this;
            }

            constexpr shared_ptr& operator=(shared_ptr& lhs)
            {
                _value.get_deleter()(_value._ptr);
                _value = lhs._value;
                (*_value._ptr._size)++;
                return *this;
            }

            constexpr shared_ptr& operator=(shared_ptr&& lhs)
            {
                _value.get_deleter()(_value._ptr);
                _value = lhs._value;
                lhs._value._ptr._size = nullptr;
                lhs._value._ptr._value = nullptr;
                return *this;
            }

            constexpr remove_array_pointer get()
            {
                return _value._ptr._value;
            }

            constexpr remove_array_pointer get() const
            {
                return _value._ptr._value;
            }

            constexpr pointer operator->()
            {
                return get();
            }

            constexpr pointer operator->() const
            {
                return get();
            }

            constexpr reference operator*()
            {
                return *get();
            }

            constexpr reference operator*() const
            {
                return *get();
            }

            constexpr size_t get_size()
            {
                return *_value._ptr_size;
            }

            constexpr bool is_unique()
            {
                return get_size() == 1;
            }
        };

        template<typename T>
        struct MakeShr
        {
            using single_object = shared_ptr<T>;
        };

        template<typename T>
        struct MakeShr<T[]>
        {
            using array = shared_ptr<T[]>;
        };

        template<typename T, size_t N>
        struct MakeShr<T[N]>
        {
            struct invalid_type {};  
        };

        template<typename T, typename... Args>
        static constexpr typename MakeShr<T>::single_object 
        make_shared(Args&&... args)
        {
            return shared_ptr<T>(new T(hsd::forward<Args>(args)...));
        }

        template<typename T>
        static constexpr typename MakeShr<T>::array 
        make_shared(size_t size)
        {
            using ptr_type = remove_array_t<T>;
            return shared_ptr<T>(new ptr_type[size]());
        }

        template<typename T, typename... Args>
        static constexpr typename MakeShr<T>::invalid_type 
        make_shared(Args&&...) = delete;
    }
}