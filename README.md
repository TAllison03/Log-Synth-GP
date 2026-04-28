# Log-Synth-GP

This repository contains a tool that uses Boolean Difference math to find internal Observability Don't Cares (ODC) and optimize logic circuits.

## File Structure

```text
Lg-Synth-GP/
├── abc/                    
│   ├── src/               
│   └── libabc.a            
├── outputs/
│   ├── ***Program Outputs***
│   └── .gitkeep
├── temp/
│   ├── *Working Files*
│   └── .gitkeep
├── tests/                    
│   ├── containsDCs/               
│   │   ├── large_odc.blif  
│   │   ├──medium_odc.blif 
│   │   └── small_odc.blif
│   └── noDCs/               
│       ├── large_odc.blif  
│       ├──medium_odc.blif 
│       └── small_odc.blif
├── func.blif        
├── impl_1.blif 
├── .gitignore              
├── Makefile               
├── optimizer.cpp          
├── optimizer.h            
├── README.md              
```

## Compilation Process

### Essential build requirements
```bash
sudo apt update && sudo apt install build-essential
```

### Clone ABC Repo inside this project
```bash
git clone https://github.com/berkeley-abc/abc.git
```

### Compile ABC Library
```bash
cd abc
make libabc.a
cd ..
```

### If SIS won't run
You might not have 32bit support. Try:
```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6:i386
```

## Usage

```bash
# Compile the optimizer
make

# Run the optimizer
./optimizer {your blif filepath}

# Clean any compiled files or temporary .blif files
make clean
```

## Additional Notes

- Many of the important functions for the optimizer were found as internal ABC functions, located in abc/src/aig/aig/aig.h
- func.blif and impl_1.blif are just two very simple example files. They are just to test functionality, not robustness.
- The tests folder contains more test circuits you can run to try the program out.
- We noticed that we get a -Wunused-function when compiling. This is from ABC's library, and while we could suppress this warning, I'd rather not suppress warnings.
- We tried running SIS ourselves with a script, but we couldn't get it to work automatically. We felt printing the exact commands and opening SIS for the user was our best bet.

Check out the [Berkeley ABC Repository](https://github.com/berkeley-abc/abc) for more info.
