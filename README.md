![GitQLient logo](https://github.com/francescmm/GitQlient/blob/master/resources/icons/GitQlientLogo96.png "GitQlient")

# GitQlient: Multi-platform Git client written with Qt
[![Build status](https://ci.appveyor.com/api/projects/status/ihw50uwdiim952c0/branch/master?svg=true)](https://ci.appveyor.com/project/francescmm/gitqlient/branch/master)
[![Build Status](https://travis-ci.org/francescmm/GitQlient.svg?branch=master)](https://travis-ci.org/francescmm/GitQlient)

GitQlient, pronounced as git+client (/gɪtˈklaɪənt/) is a multi-platform Git client originally forked from QGit. Nowadays it just keeps some old functionality to store the data that is used to later paint the repo tree.

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

## Sreenshots
Here are some screenshots of the first release to show you how it looks like. The main window is formed by the repository graph, information about the branches, tags and submodules, and detailed description of a commit or the current work in progress:
![GitQlient main window](https://www.francescmm.com/wp-content/uploads/2020/02/image.png "GitQlient main window")

By using the context menu over or with double-clicking the file name in the commit info panel or in the work in progress panel, you will be able to see the diff of that specific file. By the other hand, if you double-click commit or go through the context menu of the repository view, you can have a diff of a commit compared to its parent. If you want to compare two different commits, just select them and check the Diff option in the context menu:
![GitQlient diff window](https://www.francescmm.com/wp-content/uploads/2020/02/image-2.png "GitQlient diff window")

Another important screen is the History&Blame window. There you can follow the history of a file, and blame it through it:
![GitQlient blame window](https://www.francescmm.com/wp-content/uploads/2020/02/image-3.png "GitQlient blame window")

## Releases

GitQlient is always under development, but you can find the releases in the [Releases page](https://github.com/francescmm/GitQlient/releases).

It is planned to release for Linux, MacOs and Windows. However, take into account that the development environment is based on Linux and it will be the first platform released.

## Params for command line execution

GitQlient can be executed from command line with additional params. Please take a look to the following table:

| Command  | Desciption  |
|---|---|
| -noLog  | Disables the log system for the current execution  |
| -logLevel | Sets the log level for GitQlient. It expects a numeric: 0 (Trace), 1 (Debug), 2 (Info), 3 (Warning), 4 (Error) and 5 (Fatal). |
| -repos  | Provides a list separated with blank spaces for the different repositories that will be open at startup. <br> Ex: ```-repos /path/to/repo1 /path/to/repo2```  |

## Setup & Building the code

GitQlient is really easy to set up and build. You just need to follow [the guide](https://github.com/francescmm/GitQlient/blob/master/docs/SETUP_BUILD.md).

## Contributions

The current contribution guidelines are in progress. They will be updated in its [own guideline](https://github.com/francescmm/GitQlient/blob/master/docs/CONTRIBUTING.md).

## Recognition

GitQlient started as a fork from QGit. Despite it has changed a lot, there is some of the original code still, mainly the Git core functionality.

Even when is 100% transformed is nice to thanks those that make the original QGit possible. Please check the QGit contributors list [on GitHub](https://github.com/feinstaub/qgit/graphs/contributors)!

The app icon is custom made, but the other in-app icons are made by [Dave Gandy](https://twitter.com/davegandy) from [FontAwesome](https://fontawesome.com/).

## License

GitQlient is released under LGPLv2+. However some parts of the old QGit are GPLv2 so for the moment the code is stacked with that.

If you are interested, here is [the license](https://github.com/francescmm/GitQlient/blob/master/LICENSE)
