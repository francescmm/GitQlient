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

This document tries to cover exactly that.

## Glossary

Here you can find all the specific glossary that will be used in this document referring to GitQlient:

- **WIP**: Work in progress. Usually refers to the local modification in files of the repository that are not committed yet.

# How to use GitQlient

As I explained in the introduction, GitQlient support multiple repositories opened at the same time. All repositories are managed in the same isolated way. All the features of GitQlient will be presented along the User Manual: all of them apply to all the opened repositories individually. Unfortunately there are no cross-repository features.

Since the beginning I divided GitQlient three big sections depending on their functionality:

- [The Tree View (or Main Repository View)](#the-three-view)
- [The Diff View](#the-diff-view)
- [The Blame & History View](#the-blame-history-view)

These views, when enabled, can be accessed by the Controls placed at the top of the repository window.

There is another view but is not accessible always: it is the [*Merge View*](#the-merge-view). This view is visible and accessible when GitQlient detects that there is a conflict caused by a merge, cherry-pick, or pull action.

## Initial screen
The first screen you will see when opening GitQlient is the *Initial screen*. It contains buttons to handle repositories and three different widgets:

- GitQlient configuration
- Most used repositories
- Recently opened repositories

![GitQlient - Initial screen](/GitQlient/assets/1_initial_screen.png "GitQlient - Initial screen")GitQlient - Initial screen

### GitQlient configuration

In the GitQlient configuration, you can change some internal parameters that GitQlient uses to update the view and internal data. The available options are:

- Auto-Fetch interval: defined in minutes (from 0 to 60) this interval is used to fetch automatically the changes in the remote repository.
- Auto-Prune: if active, GitQlient will perform prune actions when it does the automatic fetch.
- Disable logs: if active, it disables GitQlient logs.
- Log level: Allows you to choose the threshold of the levels that GitQlient will write. The higher level, the lesser amount of logs.
- Auto-format files (not operative): if active, every time that you make a commit, it will perform an auto-formating of the code. The formatting will be done by using clang and the clang-format file defined at the root of the repository.

### Initializing a new repository

To create a new local repository you have to click over the option *Init new repo*. It opens a dialog to set all the information that Git needs. This is:

- Destination of the repository (where it will be stored locally)
- Repository name: the name of the repository (refers to the folder name of the project)

In addition, you can configure the Git user by checking the checkbox GitQlient will open the repository after it's created.

![GitQlient - Init new repo](/GitQlient/assets/1_init_repo.png "GitQlient - Init new repo")GitQlient - Init new repo

### Cloning a remote repository

To clone an existing remote repository you have to select the option *Clone new repo*. After clicking the button, it will show a dialog that ask for all the necessary data that Git needs to clone a repo. That is:

- Repository destination: where the repository will be stored.
- URL: The remote URL for the repository.
- Repository name: it's automatically filled with the repo name from the URL, but can be changed if wanted.

In addition, there are two options after the clone action takes place:

- Checkbox to open the repo in GitQlient.
- Checkbox to store the user data for this repository.

![GitQlient - Clone repository](/GitQlient/assets/1_clone_repo.png "GitQlient - Clone repository")GitQlient - Clone repository

### Open an existing repository

If you want to open an already cloned repository, the button *Open existing repo" openes the file explorer of the OS to select the folder that contains the repository:

![GitQlient - Open repository](/GitQlient/assets/1_open_repo.png "GitQlient - Open repository")GitQlient - Open repository

In addition to this, you can select any of the projects listed in the *Most used repos* list or in the *Recent repos" list:

![GitQlient - Open repository](/GitQlient/assets/1_open_repo_2.png "GitQlient - Open repository")GitQlient - Open repository

## Quick access actions

Once you have selected and opened our repo, the new view shows in first place a series of controls to manage the most used actions done in Git. This controls are organized horizontally as sqaured buttons as the following image shows:

![GitQlient - Quick access actions](/GitQlient/assets/2_quick_access_actions.png "GitQlient - Quick access actions")GitQlient - Quick access actions

The first three buttons reference the different views of GitQlient. They allow you to navigate GitQlient in a simple and easy way. The button changes its color when the view it refers is being dispayed:

- View: This is the main view and shows the tree view, information about the commits, the WIP, branches, tags, stashes and submodules.
- Diff: This options is disabled by default and is only active when a diff is opened. When active, it shows the opened diffs we have.
- Blame: The blame option shows the view there you can see the commit history of any file, the blame for each selected file and a view of the files in the current repository folder.

After that, you can find three buttons that trigger three of the most used Git commands. These are placed here to make you easier to sync the data between the remote and the local repository. Some buttons have an arrow that indicates that the buttons have several options. Press the arrow to select the desired Git command:

- Pull: By default, it performs a Git Pull command. When the dropdown menu is pressed you will find find other options:

![GitQlient - Pull options](/GitQlient/assets/2_pull_options.png "GitQlient - Pull options")GitQlient - Pull options

    - Fetch all: Fetches branches, commits and tags. If your current branch is behind the remote branch after fetching, GitQlient will ask if you want to pull the new changes.
    - Pull: This is the default behaviour.
    - Prune: Prunes all remote deleted tags and branches.

- Push: It performs the regular push (not *forced*) command.
- Stash: It does not have a default command. Instead you have to press the dropdown menu to see the different options:

![GitQlient - Stash options](/GitQlient/assets/2_stash_options.png "GitQlient - Stash options")GitQlient - Stash options

    - Stash push: Pushes the stash to your local repository.
    - Stash pop: Pops the latest stash that you pushed.

- Refresh: This option performs a deep refresh of the repository cache. It reloads cache, views and branches information. This is costly so please take it into account when you trigger it. It's usually helpful to use if you have performed Git actions outside GitQlient and you want to sync.
- Config: The last option opens the repository config dialog. For now, it shows the user data for the current repository:

![GitQlient - Repository config](/GitQlient/assets/2_repo_config.png "GitQlient - Repository config")GitQlient - Repository config

The repository configuration dialog shows the configuration of your .gitconfig file. For the moment only the options about the current user are displayed. You can modify them, of course.

## <a name="the-three-view"></a>The Tree View

![GitQlient - The Tree View](/GitQlient/assets/3_the_tree_view.png "GitQlient - The Tree View")GitQlient - The Tree View

The tree view is divided in three different sections:
* In the center you can find the graphic representation of the repository tree.
* In the right side, GitQlient displayes information about the local & remote branches, tags, stashes and submodules.
* In the left side, GitQlient shows the information about the commit you select in the tree view. It will vary depending on if you select the work in progress or a commit.

### The repository graph tree

The repository graph tree is as it's name says: the graphical representation in a form of a tree of the state of your repository. It shows all the branches with their branch names, tags and stashes.

By default, the order is done by date but in future release will be configurable.

In the top of the view you can find a long control to input text. There you can search a specific commit by its SHA or by the commit message. At the end of the input control, you will find a checkbox that when it's active the view shows all the branches. In case you want to work only with your current checked out branch, you can uncheck it and the view will be updated.

#### Commit selection

The tree view supports multi-selection and the context menu will vary depending on how many commits you select. The different actions you can do to a commit are:

* Double click a commit: It opens the commit diff between the double clicked one and its direct parent.
* Single click: allows multiple selection by using the Shift key (range selection between 2 commits cliked), accumulative individual selection with the Control key, and click and slide that will select all the commits between the press and the release of the mouse.

If you select two commits, you will be able to see the diff between them by selecting that option in the contextual menu.

Over the selection you can perform different actions:

* On commit selected:
    - If the commit is the last one you will find the following options:
    ![GitQlient - Options for last commit](/GitQlient/assets/3_current_options.png "GitQlient - Options for last commit")GitQlient - Options for last commit
    - If the selection is the work in progress:
    ![GitQlient - WIP options](/GitQlient/assets/3_wip_options.png "GitQlient - WIP options")GitQlient - WIP options
    - If the commit is the last commit of a different branch:
    ![GitQlient - Branch commit options](/GitQlient/assets/3_branch_options.png "GitQlient - Branch commit options")GitQlient - Branch commit options
    - If the commit select is in a different branch and is not the last one, you will have the same options that before but without the *Checkout branch...* and *Merge* options.

### WIP view

When you select the first entry in the graphic tree view when the text says *Local changes*, it will show the information of your local uncommited changes in a widget on the left side of the graphic view:

![GitQlient - WIP view](/GitQlient/assets/3_wip_view.png "GitQlient - WIP view")GitQlient - WIP view

This view is divided in four sections. The first list shows the files that are untracked in your local repository. The second list shows the files that have local modifications. Following that you will find the third list with the changes that are already added to the next commit. Finally in the bottom of the view, you have two input controls where you can add the title of the commit (up to 50 characters), the description for long explanatory texts and a button to commit.

To change the status of a file you can press the plus/minus button or open the contextual menu. The contextual menu will vary depending on the view:

Untracked options:
* Stage file: Moves the file to the stage list.
* Delete file: Deletes the file **without** confirmation. The reason is that it's an action not that common and you can recover the file most of the time.

Unstaged options:
* See changes: Opens the diff view with the changes between the current work and the last commit.
* Blame: Opens the blame and history view showing the selected file.
* Stage file: Moves the file to the stage list.
* Revert file changes: Reverts all the changes of the file selected.
* Ignore file: Adds the file name to the ignore list of Git.
* Ignore extension: Adds the file extension to the ignore list of Git.
* Add all files to commit: Moves all files in the list to the staged list.
* Revert all changes: Reverts all the changes in all the files.

Staged options:
* Unstage file: Moves the file to its previous list. When amending it moves the file to the unstaged list.
* See changes: Opens the diff view with the changes between the current work and the last commit.

#### Amending a commit

The same view applies when you want to amend a commit. The only difference is that the title and description will be filled with the information from the commit you are amending, and the button will change its text to *Amend*.

### Commit info view

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
