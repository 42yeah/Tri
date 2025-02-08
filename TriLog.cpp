#include "TriLog.hpp"

// Verbose

template <> void TriLogger<TriLogVerbose>::Tag()
{
    std::cout << "[" << TRI_LOG_APP << "] [D] ";
}

template <> void TriLogger<TriLogVerbose>::Flush() { std::cout << std::endl; }

// Info

template <> void TriLogger<TriLogInfo>::Tag()
{
    std::cout << "[" << TRI_LOG_APP << "] [I] ";
}

template <> void TriLogger<TriLogInfo>::Flush() { std::cout << std::endl; }

// Warning

template <> void TriLogger<TriLogWarning>::Tag()
{
    std::cerr << "[" << TRI_LOG_APP << "] [W] ";
}

template <> void TriLogger<TriLogWarning>::Flush() { std::cerr << std::endl; }

// Error

template <> void TriLogger<TriLogError>::Tag()
{
    std::cout << "[" << TRI_LOG_APP << "] [E] ";
}

template <> void TriLogger<TriLogError>::Flush() { std::cerr << std::endl; }

