# GitQlient
[![Build status](https://ci.appveyor.com/api/projects/status/ihw50uwdiim952c0/branch/master?svg=true)](https://ci.appveyor.com/project/francescmm/gitqlient/branch/master)
[![Build Status](https://travis-ci.org/francescmm/GitQlient.svg?branch=master)](https://travis-ci.org/francescmm/GitQlient)

GitQlient is Git client that was forked from QGit. Nowadays still have some old functionality mainly in the core parts.

GitQlient is not only a refactor of the UI but it also does a big refactor on the core parts. The original plan was to modernize the UI, move to C++17 standard and remove old bad practices to be compliant with the C++ Guidelines.

Following there are some of the new features:

1. Better UI experience
    1. Easy access to the main Git actions
    2. Better code separation between Views and Models
    3. Simplification of the different options we have keeping it to what a Git client is
2. New features:
    1. Easy access to remote actions like: push, pull, submodules management and branches
    2. Branches management
    3. Tags and stashes management
    4. Submodules handling
    5. Allow to open several repositories in the same window
    6. Better visualization of the commits and the work in progress
    7. Better visualization of the repository view

## Releases

Right now GitQlient is under development. That means that there are no stable releases so far.

Releases will be marked with a tag and they will be shown in the [Releases page](https://github.com/francescmm/GitQlient/releases). It is planned to release for Linux, MacOs and Windows. However, take into account that the development environment is based on Linux and it will be the first platform released.

## Setup & Building the code

### Pre-requisites

#### Qt

GitQlient is being developed with [Qt 5.13](https://www.qt.io/download-qt-installer). Despite is not tested, any versions from 5.9 should be ok.

The plan for the near future is to test for every major version from 5.9 to the latest.

#### Git

Since GitQlient it's a Git client, you will need to have Git installed and added to the path.

### Submodules

For now, GitQlient has only one dependency and that's [QLogger](https://github.com/francescmm/QLogger).

Remember that the first time you will need to initialize the submodule and update it from time to time.

### Steps

If you just want to play with it a bit with GitQlient or just build it for your own environment, you will need to do:

1. Clone the repository:

    ```git clone https://github.com/francescmm/GitQlient.git ```

2. Go into the GitQlient project folder and initialize the submodules:

    ```git submodule update --init --recursive ```

3. Or use QtCreator or run qmake in the main repository folder (where GitQlient.pro is located):

    ```qmake GitQlient.pro ```

    If you want to build GitQlient in debug mode, write this instead:

    ```qmake CONFIG+=debug GitQlient.pro```

4. Run make in the main repository folder to compile the code:

    ```make```

## Contributions

The current contribution guidelines are in progress. They will be updated in its [own guideline](https://github.com/francscmm/GitQlient/blob/master/CONTRIBUTING.md).

## Recognition

GitQlient started as a fork from QGit. Despite it has changed a lot, there is some of the original code still, mainly the Git core functionality.

Even when is 100% transformed is nice to thanks those that make the original QGit possible. Please check the QGit contributors list [on GitHub](https://github.com/feinstaub/qgit/graphs/contributors)!

## License

GitQlient is released under LGPLv2+. However some parts of the old QGit are GPLv2 so for the moment the code is stacked with that.

If you are interested, here is [the license](https://github.com/francscmm/GitQlient/blob/master/LICENSE)
