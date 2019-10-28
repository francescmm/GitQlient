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

## Params for command line execution

GitQlient can be executed from command line with additional params. Please take a look to the following table:

| Command  | Desciption  |
|---|---|
| -noLog  | Disabled the log system for the current execution  |
| -repos  | Provides a list separated with blank spaces for the different repositories that will be open at startup. <br> Ex: ```-repos /path/to/repo1 /path/to/repo2```  |

## Setup & Building the code

GitQlient is really easy to set up and build. You just need to follow [the guide](https://github.com/francscmm/GitQlient/blob/master/SETUP_BUILD.md).

## Contributions

The current contribution guidelines are in progress. They will be updated in its [own guideline](https://github.com/francscmm/GitQlient/blob/master/CONTRIBUTING.md).

## Recognition

GitQlient started as a fork from QGit. Despite it has changed a lot, there is some of the original code still, mainly the Git core functionality.

Even when is 100% transformed is nice to thanks those that make the original QGit possible. Please check the QGit contributors list [on GitHub](https://github.com/feinstaub/qgit/graphs/contributors)!

## License

GitQlient is released under LGPLv2+. However some parts of the old QGit are GPLv2 so for the moment the code is stacked with that.

If you are interested, here is [the license](https://github.com/francscmm/GitQlient/blob/master/LICENSE)
