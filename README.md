# Log-Synth-GP

This repository contains a tool that uses Boolean Difference math to find internal Observability Don't Cares (ODC) and optimize logic circuits.

## File Structure

Lg-Synth-GP/
|--- optimizer.cpp
|--- optimizer.h
|--- func.blif
|--- impl_1.blif
|--- abc/
     |--- libabc.a
     |--- src/

## Compilation Process

### Clone ABC Repo
```bash
git clone https://github.com/berkeley-abc/abc.git
```

### Compile ABC Library
```bash
cd abc
make libabc.a
cd ..
```

## Usage

```bash
# Compile the optimizer
make

# Run teh optimizer
./optimizer

# Clean any compiled files or temporary .blif files
make clean
```


## Additional Notes

- Many of the key macros and functions for the optimizer were found as internal ABC functions, located in abc/src/aig/aig/aig.h
- The .blif file used for optimization needs to be hardcoded in the current version. Simply change the string in optimizer.cpp, line 98
- func.blif and impl_1.blif are just two very simple example files. They are just to test functionality, not robustness.

Check out the [Berkeley ABC Repository](https://github.com/berkeley-abc/abc) for more info.
