#ifndef HPC_HELPERS_HPP
#define HPC_HELPERS_HPP

#include <iostream>
#include <fstream>
#include <cstdint>

#ifndef __CUDACC__
    #include <chrono>
#endif

#ifndef __CUDACC__
    #define TIMERSTART(label, unit, unitstr, outputStream, initString)         \
        std::chrono::time_point<std::chrono::system_clock> a##label, b##label; \
        a##label = std::chrono::system_clock::now();
#else
    #define TIMERSTART(label, unit, unitstr, outputStream, initString)         \
        cudaEvent_t start##label, stop##label;                                 \
        float time##label;                                                     \
        cudaEventCreate(&start##label);                                        \
        cudaEventCreate(&stop##label);                                         \
        cudaEventRecord(start##label, 0);
#endif

#ifndef __CUDACC__
    #define TIMERSTOP(label, unit, unitstr, outputStream, initString)          \
        b##label = std::chrono::system_clock::now();                           \
        std::chrono::duration<double> delta##label = b##label-a##label;        \
        auto elapsedTime = unit * delta##label.count();                        \
        if (initString == NULL)                                                \
            outputStream << "# elapsed time (" << #label << "): "              \
                  << elapsedTime  << " (" << unitstr << ")" << std::endl;      \
        else                                                                   \
            outputStream << initString << elapsedTime                          \
                  << unitstr << std::endl;                      
#else
    #define TIMERSTOP(label, unit, unitstr, outputStream, initString)              \
            cudaEventRecord(stop##label, 0);                                       \
            cudaEventSynchronize(stop##label);                                     \
            cudaEventElapsedTime(&time##label, start##label, stop##label);         \
            if (initString == NULL)                                                \
                outputStream << "TIMING: " << 0.001 * unit * time##label <<        \
                unitstr << "(" << #label << ")";                                   \
            else                                                                   \
                outputStream << 0.001 * unit * time##label <<                      \
                unitstr << initString;                                
#endif


#ifdef __CUDACC__
    #define CUERR {                                                            \
        cudaError_t err;                                                       \
        if ((err = cudaGetLastError()) != cudaSuccess) {                       \
            std::cout << "CUDA error: " << cudaGetErrorString(err) << " : "    \
                      << __FILE__ << ", line " << __LINE__ << std::endl;       \
            exit(1);                                                           \
        }                                                                      \
    }

    // transfer constants
    #define H2D (cudaMemcpyHostToDevice)
    #define D2H (cudaMemcpyDeviceToHost)
    #define H2H (cudaMemcpyHostToHost)
    #define D2D (cudaMemcpyDeviceToDevice)
#endif

// safe division
#define SDIV(x,y)(((x)+(y)-1)/(y))

// no_init_t
#include <type_traits>

template<class T>
class no_init_t {
public:

    static_assert(std::is_fundamental<T>::value &&
                  std::is_arithmetic<T>::value, 
                  "wrapped type must be a fundamental, numeric type");

    //do nothing
    constexpr no_init_t() noexcept {}

    //convertible from a T
    constexpr no_init_t(T value) noexcept: v_(value) {}

    //act as a T in all conversion contexts
    constexpr operator T () const noexcept { return v_; }

    // negation on value and bit level
    constexpr no_init_t& operator - () noexcept { v_ = -v_; return *this; }
    constexpr no_init_t& operator ~ () noexcept { v_ = ~v_; return *this; }

    // prefix increment/decrement operators
    constexpr no_init_t& operator ++ ()    noexcept { v_++; return *this; }
    constexpr no_init_t& operator -- ()    noexcept { v_--; return *this; }

    // postfix increment/decrement operators
    constexpr no_init_t operator ++ (int) noexcept {
       auto old(*this);
       v_++; 
       return old; 
    }
    constexpr no_init_t operator -- (int) noexcept {
       auto old(*this);
       v_--; 
       return old; 
    }

    // assignment operators
    constexpr no_init_t& operator  += (T v) noexcept { v_  += v; return *this; }
    constexpr no_init_t& operator  -= (T v) noexcept { v_  -= v; return *this; }
    constexpr no_init_t& operator  *= (T v) noexcept { v_  *= v; return *this; }
    constexpr no_init_t& operator  /= (T v) noexcept { v_  /= v; return *this; }

    // bit-wise operators
    constexpr no_init_t& operator  &= (T v) noexcept { v_  &= v; return *this; }
    constexpr no_init_t& operator  |= (T v) noexcept { v_  |= v; return *this; }
    constexpr no_init_t& operator  ^= (T v) noexcept { v_  ^= v; return *this; }
    constexpr no_init_t& operator >>= (T v) noexcept { v_ >>= v; return *this; }
    constexpr no_init_t& operator <<= (T v) noexcept { v_ <<= v; return *this; }

private:
   T v_;
};

#endif
