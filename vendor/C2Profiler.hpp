#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <iomanip>
#include <cmath>
#include <string>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
    #include <windows.h>
#elif defined(__linux__)
    #include <pthread.h>
    #include <sched.h>
#endif

namespace C2Core {

bool pinThreadToCore(int coreId) {
#if defined(_WIN32) || defined(_WIN64)
    return SetThreadAffinityMask(GetCurrentThread(), 1ULL << coreId) != 0;
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#else 
    return false;
#endif
}

enum class TimeUnit {
    Seconds,
    Milliseconds,
    Microseconds,
    Nanoseconds,
    Cycles
};

class RdtscProfiler {
public:
    void start() {
        _mm_lfence();
        startTime = __rdtscp(&startCore);
        _mm_lfence();
    }

    void end() {
        unsigned int temp;
        endTime = __rdtscp(&temp);
        endCore = temp;
        _mm_lfence();
        if (startCore != endCore) {
#ifndef NDEBUG
            std::cerr << "[Warning] CPU core migration detected! Start core: " 
                      << startCore << ", End core: " << endCore 
                      << ". Measurement may be inaccurate.\n";
#endif
        }
    }

    uint64_t elapsed() const {
        return endTime - startTime;
    }
private:
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t startCore = 0;
    uint32_t endCore = 0;
};

class ScopedProfiler {
public:
    ScopedProfiler(std::string name, TimeUnit timeUnit = TimeUnit::Milliseconds) 
        : name(std::move(name)), ticks(nullptr), timeFormat(timeUnit) {
        profiler.start();
    }

    ScopedProfiler(uint64_t& ticks, TimeUnit timeUnit = TimeUnit::Milliseconds) 
        : ticks(&ticks), timeFormat(timeUnit) {
        profiler.start();
    }

    ~ScopedProfiler() noexcept {
        profiler.end();
        if (ticks != nullptr) {
            *ticks = convertCycles(profiler.elapsed(), timeFormat);
        } else {
            std::cout << "[Profiler] " << name << " took " 
                      << convertCycles(profiler.elapsed(), timeFormat) 
                      << getUnitName(timeFormat) << ".\n";
        }
    }

private:
    static uint64_t& getFrequency() {
        static uint64_t freq = calibrateFrequency();
        return freq;
    }

    static uint64_t& getOverhead() {
        static uint64_t ovh = calibrateOverhead();
        return ovh;
    }

    uint64_t convertCycles(uint64_t cycles, TimeUnit format) {
        uint64_t overhead = getOverhead();
        uint64_t frequency = getFrequency();
        
        cycles = cycles < overhead ? 0 : cycles - overhead;

        switch (format) {
            case TimeUnit::Cycles:       return cycles;
            case TimeUnit::Seconds:      return cycles / frequency;
            case TimeUnit::Milliseconds: return cycles * 1000 / frequency;
            case TimeUnit::Microseconds: return cycles * 1000000 / frequency;
            case TimeUnit::Nanoseconds:  return static_cast<uint64_t>((static_cast<long double>(cycles) * 1'000'000'000L) / static_cast<long double>(frequency));
            default:                     return 0;
        }
    }

    const char* getUnitName(TimeUnit format) {
        switch (format) {
            case TimeUnit::Cycles:       return " cycles";
            case TimeUnit::Seconds:      return " s";
            case TimeUnit::Milliseconds: return " ms";
            case TimeUnit::Microseconds: return " us";
            case TimeUnit::Nanoseconds:  return " ns";
            default:                     return "";
        }
    }

    static uint64_t calibrateFrequency() {
        auto startTime = std::chrono::steady_clock::now();
        _mm_lfence();
        uint64_t startCycles = __rdtsc();
        _mm_lfence();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        _mm_lfence();
        uint64_t endCycles = __rdtsc();
        _mm_lfence();
        auto endTime = std::chrono::steady_clock::now();

        auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
        if (elapsedNs == 0) return 1;

        return ((endCycles - startCycles) * 1'000'000'000ULL) / elapsedNs;
    }

    static uint64_t calibrateOverhead() {
        uint64_t minOverhead = std::numeric_limits<uint64_t>::max();
        for (int i = 0; i < 100; i++) {
            RdtscProfiler p;
            p.start();
            p.end();
            uint64_t elapsed = p.elapsed();
            if (elapsed < minOverhead) {
                minOverhead = elapsed;
            }
        }
        return minOverhead;
    }

    std::string name;
    RdtscProfiler profiler;
    uint64_t* ticks;
    TimeUnit timeFormat;
};

class StatsAccumulator {
public:
    StatsAccumulator() {
    }

    void add(uint64_t cycles) {
        counter++;
        sum += cycles;
        if (cycles > max) {
            max = cycles;
        }
        if (cycles < min) {
            min = cycles;
        }
        double delta = cycles - mean;
        mean += delta / counter;
        double delta2 = cycles - mean;
        m2 += delta * delta2;
    }

    double getVariance() const {
        return counter ? m2 / counter : 0.0;
    }

    uint64_t getMin() const { return min; }
    uint64_t getMax() const { return max; }
    uint64_t getCounter() const { return counter; }
    double getMean() const { return mean; }


    private:
    uint64_t counter = 0;
    uint64_t sum = 0;
    uint64_t min = std::numeric_limits<uint64_t>::max();
    uint64_t max = std::numeric_limits<uint64_t>::min();
    double mean = 0.0;
    double m2 = 0.0;
};

template<typename Func>
StatsAccumulator runBenchmark(uint32_t steps, Func func) {
    StatsAccumulator stats;
    for (size_t i = 0; i < 100; i++) { func(); }
    
    uint64_t cycles = 0;
    for (uint32_t i = 0; i < steps; i++) {
        {
            ScopedProfiler profiler(cycles, TimeUnit::Cycles);
            func();
        }
        stats.add(cycles);
    }
    return stats;
}

class BenchmarkReporter {
public:
    static void print(const std::string& name, const StatsAccumulator& stats) {
        double stdDev = std::sqrt(stats.getVariance());

        std::cout << "=================================================\n";
        std::cout << " Benchmark:  " << name << "\n";
        std::cout << " Iterations: " << stats.getCounter() << "\n";
        std::cout << "-------------------------------------------------\n";
        
        std::cout << std::left << std::setw(15) << " Min:"    << stats.getMin() << " cycles\n";
        std::cout << std::left << std::setw(15) << " Max:"    << stats.getMax() << " cycles\n";
        std::cout << std::left << std::setw(15) << " Mean:"   << stats.getMean() << " cycles\n";
        std::cout << std::left << std::setw(15) << " StdDev:" << stdDev << " cycles\n";
        
        std::cout << "=================================================\n\n";
    }
};

} // namespace C2Core