#ifndef EPC_SMALL_VECTOR
#define EPC_SMALL_VECTOR

#include <cstddef>
#include <memory>

namespace epc {

    template<typename T, size_t N>
    class small_vector {
        T *data_;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type buf_[N];
        size_t capacity_, size_;
    public:
        small_vector() : data_(nullptr), capacity_(N), size_(0) {}

        small_vector(const small_vector &other) : small_vector() { // copy constructor
            if (other.size_ == 0) return;
            data_ = (T *) ::operator new(other.size_ * sizeof(T));
            capacity_ = other.size_;
            try {
                if (other.capacity_ == N) {
                    std::uninitialized_copy(other.buf_, other.buf_ + other.size_, buf_);        //todo use iterators
                } else {
                    std::uninitialized_copy(other.data_, other.data_ + other.size_, data_);     //todo use iterators
                }
            } catch (...) {
                ::operator delete(data_);
                throw;
            }
            size_ = other.size_;
        }

        small_vector(small_vector &&other) noexcept: // move constructor
        buf_(other.buf_),
        data_(other.data_),
        capacity_(other.capacity_),
        size_(other.size_)
        {
            other.data_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
        }

        small_vector &operator=(const small_vector &other) {
            if (capacity_ < other.size_) {
                small_vector temp(other);
                swap(temp);
            } else {
                if (this != &other) {
                    clear();
                    if(other.size_ <= N){
                        std::uninitialized_copy(other.data_, other.data_ + other.size_, (T*)&buf_);      //todo use iterators
                    }else{
                        std::uninitialized_copy(other.data_, other.data_ + other.size_, data_);     //todo use iterators
                    }
                    size_ = other.size_;
                }
            }
            return *this;
        }

        small_vector &operator=(small_vector &&other) noexcept {
            swap(other);
            other.clear();
            return *this;
        }

        ~small_vector() {
            clear();
            ::operator delete(data_);
        }

        void swap(small_vector &other) noexcept {
            std::swap(buf_, other.buf_);
            std::swap(data_, other.data_);
            std::swap(capacity_, other.capacity_);
            std::swap(size_, other.size);
        }

        void reserve(size_t new_capacity){
            if (new_capacity <= capacity_) return;
            // capacity_ is always >= N, therefore we need to use the dynamically-allocated memory
            T* data = (T*)::operator new(new_capacity * sizeof(T));
            size_t i = 0;
            try {
                // if capacity == N, we need to move elements from the buf_, not the data_
                if(N == capacity_){
                    for ( ; i < size_; i++) new (data + i) T(std::move_if_noexcept(((T*)&buf_)[i]));
                }else{
                    for ( ; i < size_; i++) new (data + i) T(std::move_if_noexcept(data_[i]));
                }
            } catch(...) {
                for ( ; i > 0; i--) {
                    (data + i - 1)->~T();
                    ::operator delete(data);
                    throw;
                }
            }
            clear();
            ::operator delete(data_);
            data_ = data;
            capacity_ = new_capacity;
            size_ = i;
        }

        void push_back(const T &value){
            emplace_back(value);
        };

        void push_back(T &&value){
            emplace_back(std::move(value));
        };

        template <typename... Ts>
        void emplace_back(Ts&&... params) {
            constexpr size_t numberOfParams = sizeof...(Ts);
            if (size_ + numberOfParams > capacity_) reserve(capacity_ == 0 ? 1 : 2 * capacity_);
            if(N == capacity_){
                new ((T*)&buf_ + size_) T(std::forward<Ts>(params)...);
            }else{
                new (data_ + size_) T(std::forward<Ts>(params)...);
            }
            size_+= numberOfParams;
        }

        int size() noexcept { return this->size_; }
        int capacity() noexcept { return this->capacity_; }

        void clear() noexcept {
            try {
                for (; size_ > 0; size_--){
                    (data_ + size_ - 1)->~T();
                }
            } catch (...) {}}

        T &operator[](size_t i) {
            if(capacity_ == N) return buf_[i];
            return data_[i];
        }

        const T &operator[](size_t i) const {
            if(capacity_ == N) return buf_[i];
            return data_[i];
        }
    };

    template<typename T, size_t N> void swap(small_vector<T,N> &a, small_vector<T,N> &b) noexcept { a.swap(b); }

} // namespace epc

#endif // EPC_SMALL_VECTOR