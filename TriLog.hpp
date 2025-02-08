#pragma once

#include <string>
#include <iostream>

#define TRI_LOG_APP "Tri"

enum ETriLoggerType
{
    TriLogUnknown,
    TriLogVerbose,
    TriLogInfo,
    TriLogWarning,
    TriLogError,
    TriLogFatal
};

template <ETriLoggerType type>
class TriLogger
{
public:
    TriLogger()
    {
        Tag();
    }

    ~TriLogger()
    {
        Flush();
    }

    template <typename T>
    TriLogger &operator<<(const T &message)
    {
        static_assert(type == TriLogUnknown, "Must specialize");
    }

private:
    void Tag()
    {
        static_assert(type == TriLogUnknown, "Must specialize");
    }
    
    void Flush()
    {
        static_assert(type == TriLogUnknown, "Must specialize");
    }
};

// Verbose

template <> void TriLogger<TriLogVerbose>::Tag();
template <> void TriLogger<TriLogVerbose>::Flush();

template <>
template <typename T>
TriLogger<TriLogVerbose> &TriLogger<TriLogVerbose>::operator<<(const T &message)
{
    std::cout << message;
    return *this;
}

// Info

template <> void TriLogger<TriLogInfo>::Tag();
template <> void TriLogger<TriLogInfo>::Flush();

template <>
template <typename T>
TriLogger<TriLogInfo> &TriLogger<TriLogInfo>::operator<<(const T &message)
{
    std::cout << message;
    return *this;
}

// Warning

template <> void TriLogger<TriLogWarning>::Tag();
template <> void TriLogger<TriLogWarning>::Flush();

template <>
template <typename T>
TriLogger<TriLogWarning> &TriLogger<TriLogWarning>::operator<<(const T &message)
{
    std::cerr << message;
    return *this;
}

// Error

template <> void TriLogger<TriLogError>::Tag();
template <> void TriLogger<TriLogError>::Flush();

template <>
template <typename T>
TriLogger<TriLogError> &TriLogger<TriLogError>::operator<<(const T &message)
{
    std::cerr << message;
    return *this;
}


#define TriLogVerbose() TriLogger<TriLogVerbose>()
#define TriLogInfo() TriLogger<TriLogInfo>()
#define TriLogWarning() TriLogger<TriLogWarning>()
#define TriLogError() TriLogger<TriLogError>()
