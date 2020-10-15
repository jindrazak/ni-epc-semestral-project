#ifndef EPC_SMALL_VECTOR
#define EPC_SMALL_VECTOR

#include <cstddef>
#include <memory>

namespace epc {

    template<typename T, size_t N>
    class small_vector {
        T *data_;
        size_t capacity_, size_;
    public:
        small_vector() : data_(nullptr), capacity_(0), size_(0) {}

        small_vector(const small_vector &other) : small_vector() { // copy constructor
            if (other.size_ == 0) return;
            data_ = (T *) ::operator new(other.size_ * sizeof(T));
            capacity_ = other.size_;
            try { std::uninitialized_copy(other.data_, other.data_ + other.size_, data_); }
            catch (...) {
                ::operator delete(data_);
                throw;
            }
            size_ = other.size_;
        }

        small_vector(small_vector &&other) noexcept: // move constructor
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
                    std::uninitialized_copy(other.data_, other.data_ + other.size_, data_);
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
            std::swap(data_, other.data_);
            std::swap(capacity_, other.capacity_);
            std::swap(size_, other.size);
        }

        void push_back(const T &value);

        void clear() noexcept { try { for (; size_ > 0; size_--) (data_ + size_ - 1)->~T(); } catch (...) {}}

        T &operator[](size_t i) { return data_[i]; }

        const T &operator[](size_t i) const { return data_[i]; }
    };

    template<typename T, size_t N> void swap(small_vector<T,N> &a, small_vector<T,N> &b) noexcept { a.swap(b); }

} // namespace epc

#endif // EPC_SMALL_VECTOR