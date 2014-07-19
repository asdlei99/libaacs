/*
 * This file is part of libaacs
 * Copyright (C) 2009-2010  hpi1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef LIBAACS_ATTRIBUTES_H_
#define LIBAACS_ATTRIBUTES_H_

#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3 ))
#    define AACS_ATTR_FORMAT_PRINTF(format,var) \
            __attribute__((__format__(__printf__,format,var)))
#    define AACS_ATTR_MALLOC __attribute__((__malloc__))
#    define AACS_ATTR_PACKED __attribute__((packed))
#else
#    define AACS_ATTR_FORMAT_PRINTF(format,var)
#    define AACS_ATTR_MALLOC
#    define AACS_ATTR_PACKED
#endif

#if defined(_WIN32)
#    if defined(__GNUC__)
#        define AACS_PUBLIC  __attribute__((dllexport))
#        define AACS_PRIVATE
#    else
#        define AACS_PUBLIC  __declspec(dllexport)
#        define AACS_PRIVATE
#    endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#    define AACS_PUBLIC  __attribute__((visibility("default")))
#    define AACS_PRIVATE __attribute__((visibility("hidden")))
#else
#    define AACS_PUBLIC
#    define AACS_PRIVATE
#endif

#if !defined(__GNUC__) || __GNUC__ < 3
#  define AACS_LIKELY(x)   (x)
#  define AACS_UNLIKELY(x) (x)
#else
#  define AACS_LIKELY(x)   __builtin_expect((x),1)
#  define AACS_UNLIKELY(x) __builtin_expect((x),0)
#endif

#endif /* LIBAACS_ATTRIBUTES_H_ */
