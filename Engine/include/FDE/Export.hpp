#pragma once

#ifdef _WIN32
#    ifdef FDEngine_EXPORTS
#        define FDE_API __declspec(dllexport)
#    else
#        define FDE_API __declspec(dllimport)
#    endif
#else
#    define FDE_API __attribute__((visibility("default")))
#endif
