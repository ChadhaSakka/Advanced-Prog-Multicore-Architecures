# Allocator Performance Testing

This project compares the performance of a lock-based and a lock-free memory allocator.

## Project Structure

- `allocator.h`: Header file with common definitions.
- `lock-based-allocator.c`: Lock-based allocator implementation.
- `lock-free-allocator.c`: Lock-free allocator implementation.
- `test-allocator.c`: Test program to evaluate both allocators.
- `Makefile`: To build the project.
- `README.md`: Instructions and information.

## Building the Project

Make sure you have GCC installed with pthread support.

```bash
make

