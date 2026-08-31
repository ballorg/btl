# Module: placement new (Ball.New)

## Overview

`Ball.New` exports the freestanding placement allocation functions declared by [include/ball/new.hpp](../../include/ball/new.hpp). Its interface is generated from the public-module [template](../../cmake/ball/module.cppm.in) and the common [module declarations](../../cmake/ball/modules.cmake). It is independent of `Ball.Types` and is also re-exported by that umbrella module.

## Public Interface

- `operator new( size_t, size_t, void * )` returns the supplied storage pointer.
- The matching placement `operator delete( void *, size_t, void * )` is a no-op used when construction fails.

## Header mode

Projects built with `BALL_ENABLE_MODULES=OFF` receive the same declarations through `<ball/new.hpp>` or the umbrella header `<ball/types.hpp>`.

## Dependencies

The module depends only on Ball's global `size_t` declaration.
