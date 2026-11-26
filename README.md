# Matrix multiplication based on Vulkan compute

This project is a simple POC to experiment with Vulkan compute shaders for matrix multiplications.

A simple Java project illustrates how the resulting library could be used.

Building the library requires CMake.
Testing the library requires Java >=22.

## Installing the dependencies

```
sudo apt install libvulkan-dev
sudo apt install glslc
```

## Building the library

Create the release directory where the library will be built:

```
cmake -S . -B release -DCMAKE_BUILD_TYPE=Release
```

Build the library:

```
cd release
make
```

## Testing the library

Go to the java directory and run the tests:

```
cd java
mvn test -Dmatmul.dir=../release
```
