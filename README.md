# New ReCT Compiler
New ReCT Compiler is a project aiming to standardize and finally properly make ReCT an established product.

The compiler is written in the C programming language and aims to target [rvm](https://github.com/hrszpuk/rvm.git)
(developed alongside this language)

Currently working on the binder step. Improvements to are always welcome to any step(that exists).
Just open a pull request :D

## Building the project
In order to build nrc you must have `make`, `gcc` and `python` installed.
These are used in the default makefile to build and generate project files.

The first step is cloning nrc:
```shell
git clone --recursive https://github.com/rect-lang/nrc.git
```
And then stepping into the source dir:
```shell
cd nrc
```
Then we recommend switching to the `dev` branch(as of writing this `main` is very out-of-date)
```shell
git checkout dev
```
Finally, it's time to run the make command:
```shell
make all
```

## Setting up for development
Setting up nrc for development is very simple.

First of all, you need the build dependencies(`make`, `gcc` and `python`) installed.

Then, just run the following commands:
```shell
# Clone NRC
git clone --recursive https://github.com/rect-lang/nrc.git
# Step into the source dir
cd nrc
# Switch to the dev branch
git checkout dev
# Generate generated files
make generate

# (optional) Switch to new dev branch:
# git checkout -B [new_branch]
```
And that's all(I believe)!