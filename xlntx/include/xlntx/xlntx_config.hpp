#pragma once

#ifndef XLNTX_STATIC
#define XLNTX_STATIC
#endif

#ifdef XLNTX_STATIC
#define XLNTX_API
#else
#ifdef _MSC_VER
#define XLNTX_API __declspec(dllimport)
#else
#define XLNTX_API
#endif
#endif
