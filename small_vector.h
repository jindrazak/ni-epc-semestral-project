#ifndef EPC_SMALL_VECTOR
#define EPC_SMALL_VECTOR

#include <memory>

namespace epc {

    template<typename T, size_t N>
    class small_vector {
        T *data_;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type buf_[N];
        size_t capacity_, size_;
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;

        small_vector() noexcept : data_(nullptr), capacity_(N), size_(0) {}

        // copy constructor
        small_vector(const small_vector &other) : small_vector() {
            if (other.size_ == 0) return;
            if(N < other.size_){
                //we need to use the 'data_'
                data_ = (T *) ::operator new(other.size_ * sizeof(T));
                try {
                    std::uninitialized_copy(other.begin(), other.end(), data_);
                } catch (...) {
                    ::operator delete(data_);
                    throw;
                }
                capacity_ = other.size_;
            }else{
                //data fits to the 'buf_'
                std::uninitialized_copy(other.begin(), other.end(), (T*)&buf_);
            }

            size_ = other.size_;
        }

        // move constructor
        small_vector(small_vector &&other) noexcept:
            data_(other.data_),
            capacity_(other.capacity_),
            size_(other.size_)
        {
            if(other.size_ <= N){
                //if moving from the 'buf_' we need to move the elements manually
                std::uninitialized_move(other.begin(), other.end(), begin());
                std::destroy(other.begin(), other.end());
            }
            other.data_ = nullptr;
            other.capacity_ = N;
            other.size_ = 0;
        }

        //converting constructor
        small_vector(std::initializer_list<value_type> init) : small_vector(){
            if (init.size() == 0) return;
            if(N < init.size()){
                //we need to use the 'data_'
                data_ = (T *) ::operator new(init.size() * sizeof(T));
                try {
                    std::uninitialized_copy(init.begin(), init.end(), data_);
                } catch (...) {
                    ::operator delete(data_);
                    throw;
                }
                capacity_ = init.size();
            }else{
                //data fits to the 'buf_'
                std::uninitialized_copy(init.begin(), init.end(), (T*)&buf_);
            }

            size_ = init.size();
        }

        //copy assignment operator
        small_vector &operator=(const small_vector &other) {
            if (capacity_ < other.size_) {
                small_vector temp(other);
                swap(temp);
            } else {
                if (this != &other) {
                    clear();
                    std::uninitialized_copy(other.begin(), other.end(), N >= other.size_ ? (T*)&buf_ : data_);
                    size_ = other.size_;
                }
            }
            return *this;
        }

        //move assignment operator
        small_vector &operator=(small_vector &&other) noexcept {
            swap(other);
            other.clear();
            return *this;
        }

        ~small_vector() {
            clear();
            ::operator delete(data_);
        }

        void swap(small_vector &other) {
            if(this->size_ > N && other.size_ > N){                         //both use 'data_'
                std::swap(data_, other.data_);
                std::swap(capacity_, other.capacity_);
                std::swap(size_, other.size_);
            }else if(this->size_ <= N && other.size_ <= N) {                //both use 'buf_'
                // swap what can be swapped
                size_t elementsToBeSwapped = std::min(this->size_, other.size_);
                for (size_t i = 0; i < elementsToBeSwapped; ++i) {
                    std::swap((*this)[i], other[i]);
                }
                // move the rest if needed
                if(this->size_ != other.size_){
                    auto from = this->size_ > other.size_ ? this : &other;
                    auto to = this->size_ > other.size_ ? &other : this;
                    size_t elementsToBeMoved = from->size_ - to->size_;
                    try {
                        for (size_t i = elementsToBeSwapped; i < from->size_; ++i) new ((T*)&to->buf_ + i) T(std::move_if_noexcept((*from)[i]));
                    } catch(...) {
                        //if exception is thrown, swap back to achieve strong exception guarantee
                        for (size_t i = 0; i < elementsToBeSwapped; ++i) std::swap((*to)[i], (*from)[i]);
                        // and destroy already constructed elements
                        for (size_t i = elementsToBeSwapped; i < elementsToBeMoved; ++i) ((T*)&to->buf_ + i)->~T();
                        throw;
                    }
                    //destroy moved elements from other
                    for (size_t i = elementsToBeSwapped; i < elementsToBeMoved; ++i) ((T*)&from->buf_ + i)->~T();
                }
                std::swap(size_, other.size_);
            }else if(this->size_ > N && other.size_ <= N) {                     //I use 'data_', other uses 'buf_'
                //move_if_noexcept other's 'buf_' to mine
                try {
                    for (size_t i = 0; i < other.size_; ++i) new ((T*)&buf_ + i) T(std::move_if_noexcept(other[i]));
                } catch(...) {
                    for (size_t i = 0; i < other.size_; ++i) ((T*)&buf_ + i)->~T();
                    throw;
                }
                //swap the rest
                std::swap(data_, other.data_);
                std::swap(capacity_, other.capacity_);
                std::swap(size_, other.size_);
            }else if(this->size_ <= N && other.size_ > N) {                     //I use 'buf_', other uses 'data_'
                //move_if_noexcept mine 'buf_' to other's
                try {
                    for (size_t i = 0; i < this->size_; ++i) new ((T*)&other.buf_ + i) T(std::move_if_noexcept(((T*)&buf_)[i]));
                } catch(...) {
                    for (size_t i = 0; i < this->size_; ++i) ((T*)&other.buf_ + i)->~T();
                    throw;
                }
                //swap the rest
                std::swap(data_, other.data_);
                std::swap(capacity_, other.capacity_);
                std::swap(size_, other.size_);
            }
        }

        void reserve(size_t new_capacity){
            if (new_capacity <= capacity_) return;
            // new_capacity is always > N, therefore we need to use the dynamically-allocated memory
            T* data = (T*)::operator new(new_capacity * sizeof(T));
            size_t i = 0;
            try {
                // if capacity == N (therefore we were using the 'buf_'), we need to move elements from the 'buf_'
                if(N == capacity_){
                    //move_if_noexcept gives us strong exception guarantee
                    for ( ; i < size_; i++) new (data + i) T(std::move_if_noexcept(((T*)&buf_)[i]));
                }else{
                    for ( ; i < size_; i++) new (data + i) T(std::move_if_noexcept(data_[i]));
                }
            } catch(...) {
                for ( ; i > 0; i--) {
                    (data + i - 1)->~T();
                }
                ::operator delete(data);
                throw;
            }
            clear();
            ::operator delete(data_);
            data_ = data;
            capacity_ = new_capacity;
            size_ = i;
        }

        void resize(size_t size, const value_type& value = value_type()) {
            if (size < size_){
                std::destroy(begin() + size, end());
                size_ = size;
            } else if (size > size_){
                if(size > capacity_) reserve(size);
                for ( ; size_ < size; size_++) new (begin() + size_) T(value);
            }
        }

        void push_back(const value_type &value){
            emplace_back(value);
        };

        void push_back(value_type &&value){
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

        void clear() noexcept {
            try {
                std::destroy(begin(), end());
                size_ = 0;
            } catch (...) {}
        }

        iterator begin() noexcept {
            return data();
        }

        iterator end() noexcept {
            return data() + size_;
        }

        const_iterator begin() const noexcept {
            return data();
        }

        const_iterator end() const noexcept {
            return data() + size_;
        }

        size_t size() const noexcept {
            return size_;
        }

        size_t capacity() const noexcept {
            return capacity_;
        }

        pointer data() noexcept {
            if(N == capacity_) return ((T*)&buf_);
            return data_;
        }

        const_pointer data() const noexcept {
            if(N == capacity_) return ((T*)&buf_);
            return data_;
        }

        reference operator[](size_t index) noexcept {
            return *(data() + index);
        }

        const_reference operator[](size_t index) const noexcept {
            return *(data() + index);
        }
    };

    template<typename T, size_t N> void swap(small_vector<T,N> &a, small_vector<T,N> &b) noexcept { a.swap(b); }

} // namespace epc

#endif // EPC_SMALL_VECTOR