# Utopia

__Utopia__ (gr., no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Powered by [DUNE](https://dune-project.org/)

| Testing System | Python Test Coverage |
| :------------: | :------------------: |
| [![pipeline status](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/pipeline.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master) | [![coverage report](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/coverage.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master) |

## Installation
Utopia is a DUNE module and thus relies on the [DUNE Buildsystem](https://www.dune-project.org/doc/installation/) for installation. If all requirements are met, Utopia is configured and built with the `dunecontrol` script:

    ./dune-common/bin/dunecontrol --module=utopia all

Follow the step-by-step instructions below for building Utopia from source.

### Cloning the repository
Utopia packs a set of third-party-packages as
[Git Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).
Thus, when cloning Utopia, use the `--recurse-submodules` option to automatically
clone the submodule repositories:

    git clone --recurse-submodules https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia.git

##### Submodules troubleshooting
After performing an update of the code base using `git pull`, or checking out a
new branch with `git checkout`, you need to update the submodules' tree
explicitly by calling

    git submodule update

This will perform a `git checkout` of the specified commit in all submodules.

If you already cloned the main repository but from a point where the submodules
were not yet present, you need to pull the submodules for the first time using:

    git submodule update --init --recursive


### Dependencies
| Software | Version | Comments |
| ---------| ------- | -------- |
| GCC | >= 7 | Full C++17 compiler support required |
| _or:_ clang | >= 6 | Full C++17 compiler support required |
| _or:_ Apple LLVM | >= 9 | Full C++17 compiler support required |
| [CMake](https://cmake.org/) | >= 3.10 | |
| pkg-config | | |
| [HDF5](https://www.hdfgroup.org/solutions/hdf5/) | >= 1.10. | |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | 0.6.2 | Included as submodule |
| [spdlog](https://github.com/gabime/spdlog) | >= 0.17.0 | Included as submodule |
| [Boost](http://www.boost.org/) | >= 1.65 | |
| [dune-common](https://gitlab.dune-project.org/core/dune-common) | master | |
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry) | master | |
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid) | master | |
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid) | master | |

#### Python
Utopia's frontend, `utopya`, uses Python >= 3.6 and some custom packages.
Earlier Python versions _might_ work, but are not tested.

| Software | Version | Purpose |
| ---------| ------- | ------- |
| [paramspace](https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace) | >= 1.0.1 | Makes parameter sweeps easy |
| [dantro](https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro) | >= 0.1 | A data loading and plotting framework |

If you have ssh-keys configured to access this GitLab, these packages and their dependencies will automatically be installed into a virtual environment.
If not, please see the troubleshooting section in the step-by-step instructions.

#### Recommended
| Software | Version | Purpose |
| ---------| ------- | ------- |
| [doxygen](http://www.stack.nl/~dimitri/doxygen/) | >= 1.8.14 | Builds the code documentation upon installation |



### Step-by-step Instructions
These instructions are intended for 'clean' __Ubuntu__ (18.04) or __macOS__ setups.

The main difference between the two systems are the package managers.
On macOS, we recommend [Homebrew](https://brew.sh/).
If you prefer to use [MacPorts](https://www.macports.org/), notice that some packages might need to be installed differently.
Ubuntu is shipped with APT.

1. __macOS only:__ Start by installing the Apple Command Line Tools by executing

        xcode-select --install

2. Install third-party packages with the package manager:

    __Ubuntu:__

        apt update
        apt install cmake doxygen gcc g++ gfortran git libboost-dev \
            libhdf5-dev libyaml-cpp-dev pkg-config python3-dev python3-pip
    
    __macOS:__

        brew update
        brew install boost cmake doxygen gcc pkg-config python yaml-cpp hdf5

3. Use [`git clone`](https://git-scm.com/docs/git-clone) to clone the
    DUNE repositories listed above into a suitable folder on your machine.
    Make sure to [`git checkout`](https://git-scm.com/docs/git-checkout)
    the correct branches (see the dependency list above).

4. Configure and build DUNE and Utopia by executing the `dunecontrol` script:

        CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release \
            -DDUNE_PYTHON_VIRTUALENV_SETUP=True \
            -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
            ./dune-common/bin/dunecontrol --module=utopia all

    Afterwards, reconfiguring and rebuilding can now also be done locally,
    instead of calling `dunecontrol`: after entering the `utopia/build-cmake`
    directory, you can call `cmake ..` or `make` directly.


#### Troubleshooting
* If you have a previous installation and the build failed, try removing *all* the `build-cmake` directories, either manually or using

        rm -r ./*/build-cmake/

* If the `dunecontrol` command failed during resolution of the Python
    dependencies it is due to the configuration routine attempting to load the
    packages via SSH. To fix this, the most comfortable solution is to register
    your SSH key with the GitLab; follow [this](https://docs.gitlab.com/ce/ssh)
    instruction to do so.  
    Alternatively, you can manually install the Python dependencies into the
    virtual environment that DUNE creates:

        ./build-cmake/run-in-dune-env pip install git+https://...

    The clone URLs can be found by following the links in the
    [dependency table](#dependencies). Note that deleting the build
    directories will also require to install the dependencies again.


### Building the Documentation
Utopia builds a Doxygen documentation from its source files. Use `dunecontrol` to execute the appropriate command:

    ./dune-common/bin/dunecontrol --only=utopia make doc

You will find the files inside the build directory `utopia/build-cmake/doc`.

### Build Types
If you followed the instructions above, you have a `Release` build which is
optimized for maximum performance. If you need to debug, you should reconfigure
the entire project with CMake by navigating to the `utopia/build-cmake` folder
and calling

    cmake -DCMAKE_BUILD_TYPE=Debug ..

Afterwards, call

    make <something>

to rebuild executables. CMake handles the required compiler flags automatically.

The build type (as most other CMake flags) persists until it is explicitly
changed by the user. To build optimized executables again, reconfigure with

    cmake -DCMAKE_BUILD_TYPE=Release ..


### Unit Tests
Utopia contains unit tests to ensure consistency by checking if class members and functions are working correctly. The tests are integrated into the GitLab Continuous Integration build process, meaning that failing tests cause an entire build to fail.

Tests can also be executed locally, to test a (possibly altered) version of Utopia *before* committing changes. To build them, execute

    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Debug" \
        ./dune-common/bin/dunecontrol --only=utopia make build_tests

and perform all tests by calling

    ARGS="--output-on-failure" ./dune-common/bin/dunecontrol --only=utopia make test

If the test executables are not built before executing `make test`, the corresponding tests will inevitably fail.

#### Grouped Unit Tests
We grouped the tests to receive more granular information from the CI system.
You can choose to perform only tests from a specific group by calling

    make test_<group>

Replace `<group>` by the appropriate testing group identifier.

Available testing groups:

| Group | Info |
| ----- | ---- |
| `core` | Backend functions for models |
| `dataio` | Backend functions for reading config files and writing data |
| `utopya` | Frontend package for performing simulations, reading and analyzing data |
| `models` | Models tests (implemented in `python/model_tests`) |
| `python` | All python tests |


## Currently implemented models
Below you find an overview over the models currently implemented in Utopia.
By clicking on the model name, you will be guided to the corresponding directory inside `dune/utopia/models`, where all models reside.
This will also show you a README of the model which contains further information about the aims and the implementation of the model.

| Model Name | Description |
| ---------- | ----------- |
| [`SimpleEG`](dune/utopia/models/SimpleEG) | A model of simple evolutionary games on grids |


#### Model templates, tests, benchmarks ...
Additionally, some models are only needed to assert that Utopia functions as desired:

| Model Name | Description |
| ---------- | ----------- |
| [`dummy`](dune/utopia/models/dummy) | Main testing model |
| [`CopyMe`](dune/utopia/models/CopyMe) | A template for creating new models _(see below)_ |
| [`CopyMeBare`](dune/utopia/models/CopyMeBare) | A _minimal_ template for creating new models |


#### New to Utopia? How do I build a model?
Aside exploring the already existing models listed above, you should check out the [Beginners Guide](dune/utopia/models/BeginnersGuide.md), which will guide you through the first steps of building your own, fancy, new Utopia model. :tada:
