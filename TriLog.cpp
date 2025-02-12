#include "TriLog.hpp"

#include "TriConfig.hpp"

#if TRI_COLORED_LOG
#define TRI_COLOR_VERBOSE "\x1b[90m"
#define TRI_COLOR_INFO "\x1b[92m"
#define TRI_COLOR_WARNING "\x1b[33m"
#define TRI_COLOR_ERROR "\x1b[91m"
#define TRI_COLOR_RESET "\x1b[0m"

#else
#define TRI_COLOR_VERBOSE ""
#define TRI_COLOR_INFO ""
#define TRI_COLOR_WARNING ""
#define TRI_COLOR_ERROR ""
#define TRI_COLOR_RESET ""

#endif

// Verbose

template <> void TriLogger<TriLogVerbose>::Tag()
{
    std::cout << TRI_COLOR_VERBOSE << "[" << TRI_LOG_APP << "] [D] ";
}

template <> void TriLogger<TriLogVerbose>::Flush()
{
    std::cout << TRI_COLOR_RESET << std::endl;
}

// Info

template <> void TriLogger<TriLogInfo>::Tag()
{
    std::cout << TRI_COLOR_INFO << "[" << TRI_LOG_APP << "] [I] ";
}

template <> void TriLogger<TriLogInfo>::Flush()
{
    std::cout << TRI_COLOR_RESET << std::endl;
}

// Warning

template <> void TriLogger<TriLogWarning>::Tag()
{
    std::cerr << TRI_COLOR_WARNING << "[" << TRI_LOG_APP << "] [W] ";
}

template <> void TriLogger<TriLogWarning>::Flush()
{
    std::cerr << TRI_COLOR_RESET << std::endl;
}

// Error

template <> void TriLogger<TriLogError>::Tag()
{
    std::cout << TRI_COLOR_ERROR << "[" << TRI_LOG_APP << "] [E] ";
}

template <> void TriLogger<TriLogError>::Flush()
{
    std::cerr << TRI_COLOR_RESET << std::endl;
}

