# Byts by Umwelt Games

A small **C++ / SFML simulation playground** for experimenting with autonomous agents.

The project explores simple creatures ("Byts") interacting with a world. The goal is to keep the codebase **small, readable, and easy to return to later**, while allowing new behaviours to be added incrementally.

Future ideas include:

* hunger-driven behaviour
* food seeking
* simple sensing
* emergent movement patterns
* small evolving ecosystems

The project is built using **CMake**, which handles configuration and compilation across platforms.

---

# Project Overview

The simulation contains a few core concepts.

### Byt

A simple autonomous agent with internal state and behaviour.

### WorldObject

Generic objects that exist in the world, such as food or obstacles.

### Sense / SenseMask

Defines what types of objects a Byt can detect.

### Simulation Loop

The simulation repeatedly performs:

```
update → decide → move → render
```

This pattern makes it easy to add new behaviours over time.

---

# Project Structure

Typical repository layout:

```
.
├── CMakeLists.txt
├── README.md
├── include/
│   └── byts/
│       └── *.hpp
├── src/
│   └── *.cpp
└── assets/
```

### Headers vs Source Files

C++ separates declarations from implementation.

**Header files (`.hpp`)**

* class definitions
* function declarations
* lightweight inline code

Think of these as describing **what exists**.

**Source files (`.cpp`)**

* actual function logic
* behaviour implementations

Think of these as describing **how things work**.

The `.cpp` files are compiled and linked into the final executable.

---

# Requirements

To build the project you need:

* a modern **C++ compiler**
* **CMake**
* **SFML**

---

# Getting Set Up

If you are returning to this project later, the workflow is usually:

1. clone the repository
2. install dependencies
3. configure the project
4. build (compile) the project
5. run the executable

---

# Clone the Repository

Download the project from GitHub:

```bash
git clone <repo-url>
cd <repo-folder>
```

Replace `<repo-url>` with the repository URL.

---

# Install Dependencies

Before building, install the required tools.

## Fedora

```bash
sudo dnf install cmake gcc-c++ SFML-devel
```

## Ubuntu / Debian

```bash
sudo apt update
sudo apt install cmake g++ libsfml-dev
```

---

# Configure the Project

Run:

```bash
cmake -S . -B build
```

This is the **configure step**.

CMake reads the `CMakeLists.txt` file and prepares the build system for your machine.
It creates a `build/` directory containing files needed for compilation.

Think of this as:

> preparing the project for compilation.

This step **does not compile the code** yet.

You usually need to run this only:

* the first time after cloning the repo
* if the `build` directory is deleted
* if important CMake settings change

---

# Build the Project (Compile)

Run:

```bash
cmake --build build -j
```

This is the **build step**.

This step actually **compiles the C++ source files** and links them into an executable.

If you are used to older terminology:

> build = compile the project

The `-j` option allows the compiler to use multiple CPU cores, which speeds up compilation.

After changing `.cpp` or `.hpp` files, this is usually the only command you need to run again.

---

# Run the Program

After a successful build, run the executable:

```bash
./build/byts_demo
```

Depending on the CMake configuration, it may also appear in:

```
./build/bin/byts
```

---

# Typical Workflow After Returning to the Project

Most of the time you will only need to run:

```bash
cmake --build build -j
./build/byts_demo
```

---

# Clean Rebuild

If the build folder becomes corrupted or confused:

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j
./build/byts_demo
```

---

# Roadmap

Short-term ideas:

```
[ ] add food objects to the world
[ ] add hunger state to Byts
[ ] hunger threshold triggers food-seeking behaviour
[ ] Byts detect nearby food
[ ] eating reduces hunger
```

---

# Notes for Future Me

Quick reminder:

```
cmake -S . -B build
cmake --build build -j
./build/byts_demo
```

If compilation fails with SFML errors, make sure the **SFML development libraries** are installed.

---

# License

TBD