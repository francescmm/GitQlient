---
layout: default
---

# GitQlient User Manual

## Introduction

GitQlient, pronounced as git+client (/gɪtˈklaɪənt/) is a multi-platform Git client originally forked from QGit. Nowadays it goes beyond of just a fork and adds a lot of new functionality.

The original idea was to provide a GUI-oriented Git client that was easy to integrate with QtCreator (currently shipped as GitQlientPugin). This idea has grown since the day 1 to not only cover the integration with QtCreator but also to make it an app on it's own.

The primarily idea behind GitQlient was to modernize the old code that QGit is based on and provide a easy UI/UX that I was actually missing on other clients. That was achieved in the version 1.0.0 of GitQlientPlugin in December. On that version, some features that were not part of QGit were included to make it easier to handle the Git repository.

After that, I felt free to open the gates for the big new features that I was actually missing in some of the Git clients I was using. Among the features I was missing, one in particular was really painful: most of the apps only allow one repository opened at the same time. That's why I decided to include the feature of **multiple repositories in the same view** as part of GitQlient since version 1.0.0.

This was one feature, but there are several other. Some of them are application-based but other's are related with the spirit of the open source.

***But, what that means exactly?***

There are several things, but I feel that two in particular are missed most of the time. The first one is a good documentation. I'm not talking only about adding Doxygen documentation to the header files or the APIs. I mean also to provide documentation about **how** the application is designed and **why** I did it in that way. I'd like that this technical documentation helps whoever wants to contribute to the project.

The second thing I've been missing is a proper User Manual. After several years in the software industry, I'm aware that one thing is writing the code of an application and another completely different is to be able to explain how to use it. When I write the code of a project or a feature, I don't need to write a User Manual to know how it works; I've created it!

But, what happens if you want to introduce as much people as possible? Well, then we need a User Manual that tells them exactly how things work. What options they have and how to deal with possible mistakes or errors.

This document tries to cover exactly this area.

## Glossary

Here you can find all the specific glossary that will be used in this document referring to GitQlient:

- **WIP**: Work in progress. Usually refers to the local modification in files of the repository that are not committed yet.

# How to use GitQlient

As we've seen in the introduction, GitQlient support multiple repositories opened at the same time. All repositories are managed in the same isolated way. All the features of GitQlient will be presented along the User Manual: all of them apply to all the opened repositories.

GitQlient is divided in three big sections split by their functionality:

- [The Tree View (or Main Repository View)](#the-three-view)
- [The Diff View](#the-diff-view)
- [The Blame & History View](#the-blame-history-view)

These views, when enabled, can be accessed by the Controls placed at the top of the repository window.

There is another view but is not accessible always: it is the [*Merge View*](#the-merge-view). This is only triggered when GitQlient detects that there is a conflict caused by a merge, cherry-pick or pull action.

## Initial screen

### GitQlient configuration

### Initializing a new repository

### Cloning a remote repository

### Open an existing repository

## Quick access actions

## <a name="the-three-view"></a>The Tree View

![GitQlient - The Tree View](https://www.francescmm.com/wp-content/uploads/2020/02/image.png "GitQlient - The Tree View")

### The repository graph tree

#### Commit selection

##### Options

#### Contextual menu options

##### Options for the WIP

##### Options for the last commit

##### Options other commits

### Viewing the changes of a commit

#### WIP view

#### Commit info view

### Branches information panel

#### Local branches

#### Remote branches

#### Stashes

#### Submodules

## <a name="the-diff-view"></a>The Diff View

![GitQlient - The Diff View](https://www.francescmm.com/wp-content/uploads/2020/02/image-2.png "GitQlient - The Diff View")

### Commit diff view

### File diff view

## <a name="the-blame-history-view"></a>The Blame & History View

![GitQlient - The Blame & History View](https://www.francescmm.com/wp-content/uploads/2020/02/image-3.png "GitQlient - The Blame & History View")

### The file view

### The blame view

### The history view

## <a name="the-merge-view"></a>The Merge View

## <a name="appendix-a-release"></a>Appendix A: Releases
GitQlient is always under development, but you can find the releases in the [Releases page](https://github.com/francescmm/GitQlient/releases).

It is planned to release for Linux, MacOs and Windows. However, take into account that the development environment is based on Linux and it will be the first platform released.

## <a name="appendix-b-build"></a>Appendix B: Build form source

### Set up & Build the code

GitQlient is being developed with the latest version of Qt, currently [Qt 5.13](https://www.qt.io/download-qt-installer). Despite is not tested, any versions from 5.12 should be okay. The plan for the near future is to test for every major version from 5.9 to the latest.

Since GitQlient it's a Git client, you will need to have Git installed and added to the path.

For now, GitQlient has only one dependency and that's [QLogger](https://github.com/francescmm/QLogger).

Remember that the first time you will need to initialize the submodule and update it from time to time.

If you just want to play with it a bit with GitQlient or just build it for your own environment, you will need to do:

1. Clone the repository:

    ```git clone https://github.com/francescmm/GitQlient.git ```

2. Go into the GitQlient project folder and initialize the submodules:

    ```git submodule update --init --recursive ```

3. Or use QtCreator or run *qmake* in the main repository folder (where GitQlient.pro is located):

    ```qmake GitQlient.pro ```

    If you want to build GitQlient in debug mode, write this instead:

    ```qmake CONFIG+=debug GitQlient.pro```

4. Run make in the main repository folder to compile the code:

    ```make```

## <a name="appendix-c-contributing"> Appendix C: Contributing
GitQlient is free software and that means that the code and the use its free! But I don't want to build something only that fits me.

I'd like to have as many inputs as possible so I can provide as many features as possible. For that reason I hope this guideline gives you an overview of how to contribute.

It doesn't matter what yo know or not, there is always a way to help or contribute. May be you don't know how to code in C++ or Qt, but UX is another field. Or maybe you prefer to provide ideas about what you would like to have.

#### Reporting errors
My intention is to use the features that GitHub provides. So the [Issues page](https://github.com/francescmm/GitQlient/issues) and the [Projects page](https://github.com/francescmm/GitQlient/projects) are two options to start with. I you prefer to report bugs or requests features, you can use the Issues tab and add a new issue with a label. [Every label](https://github.com/francescmm/GitQlient/labels) has a description but if you're not sure, don't worry, you can leave it empty. The current labels are:

- Rookie task: Perfect development task to start to know GitQlient
- Task: Task that should be solved in a single commit but is not a rookie task
- Feature: I want this amazing feature!
- Improvement: Extend functionality or improve performance of a specific topic
- Critical bug: Bug that makes GitQlient to crash
- Bug: Bug that makes GitQlient unstable. It doesn't crash
- Documentation: The issue is only about documentation
- Ready to review: The issue is ready to review

As soon as I see need of more I'll add them.

If you want to report a bug, please make sure to verify that it exists in the latest commit of master or in the current version.

#### Implementing features or fixing bugs
If you want to implement a new feature or solve bugs in the Issues page, you can pick it up there and start coding!

If you're familiar with Qt and/or C++, you can go directly to the [features](https://github.com/francescmm/GitQlient/labels/Feature) or the [bugs](https://github.com/francescmm/GitQlient/labels/Bug). Otherwise, the [rookie tasks](https://github.com/francescmm/GitQlient/labels/Rookie%20task) are a nice way to start.

First of all, you must [fork GitQlient](https://help.github.com/en/github/getting-started-with-github/fork-a-repo) and clone into your computer. In addition, you must have configured the original GitQlient repository to upstream:

```git remote add upstream https://github.com/francescmm/GitQlient.git```

When you start with an issue, make sure you create a branch from master:

```git checkout dev
git pull upstream dev
git checkout -b nameOfMyBranch
```

Ideally, your branch name should have the following schema:

- feature/short-feature-title
- bug/short-bug-title
- critical/short-critical-bug-title

Following these formats makes it a lot easier to know what you want to achieve and who is the responsible and aid us in getting contributions integrated as quickly as possible. Remember to follow the Code styles and the Code guidelines.

Once you are done with your changes and you have pushed them to your branch, you can create a [Pull Request](https://github.com/francescmm/GitQlient/pulls). Remember to add a good title and description. And don't forget to add the label!

#### Code style

GitQlient follows the [Qt Code Style](https://wiki.qt.io/Qt_Coding_Style) as well as the [Coding Conventions](https://wiki.qt.io/Qt_Coding_Style) when they are not against the C++ Core Guidelines. In fact, there is a .clang-format file in the GitQlient repository you can use to format your code.

I don't mind that you have your own style when coding: it's easier to do it with your own style! However, before the code goes to the Pull Request you should format it so it looks as the code in the repo.

#### Code guidelines
Some time ago, [Bjarne Stroustrup](http://www.stroustrup.com) and [Herb Sutter](http://herbsutter.com/) started an amazing project called [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). I know that is a large document and I don't expect that anybody reads the whole thing or memorizes it. I won't.

But in case of doubt, is the place where we should look on how to do things or why some things are done in the way they are. Having this kind of guidelines is the best way to avoid gut feelings regarding code.

## <a name="appendix-d-recognition"></a>Appendix D: Recognition
GitQlient started as a fork from QGit. Despite it has changed a lot, there is some of the original code still, mainly the Git core functionality.

Even when is 100% transformed is nice to thanks those that make the original QGit possible. Please check the QGit contributors list [on GitHub](https://github.com/feinstaub/qgit/graphs/contributors)!

The app icon is custom made, but the other in-app icons are made by [Dave Gandy](https://twitter.com/davegandy) from [FontAwesome](https://fontawesome.com/).

## <a name="appendix-e-License"></a>Appendix E: License
*GitQlient is an application to manage and operate one or several Git repositories. With GitQlient you will be able to add commits, branches and manage all the options Git provides.*

*Copyright (C) 2020  Francesc Martinez*

*This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.*

*This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.*

*You should have received a copy of the GNU Lesser General Public License along with this library; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA*

This is an extract of the license. To read the full text, please check it [here](https://github.com/francescmm/GitQlient/blob/master/LICENSE).