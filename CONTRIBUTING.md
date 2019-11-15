# Contributions
GitQlient is free software and that means that the code and the use its free! But I don't want to build something only that fits me.

I'd like to have as many imputs as possible so I can provide as many features as possible. For that reason I hope this guideline gives you an overview of how to contribute.

It doesn't matter what yo know or not, there is allways a way to help or contribute. May be you don't know how to code in C++ or Qt, but UX is another field. Or maybe you prefer to provide ideas about what you would like to have.

## Reporting errors
My intention is to use the features that GitHub provides. So the [Issues page](https://github.com/francescmm/GitQlient/issues) and the [Projects page](https://github.com/francescmm/GitQlient/projects) are two options to start with. I you prefer to report bugs or requests features, you can use the Issues tab and add a new issue with a label. [Every label](https://github.com/francescmm/GitQlient/labels) has a description but if you're not sure, don't worry, you can leave it empty. The current labels are:

- Rookie task: Perfect dev task to start to know GitQlient
- Task: Task that should be solved in a single commit but is not a rookie task
- Feature: I want this amazing feature!
- Improvement: Extend functionality or improve performance of a specific topic
- Critical bug: Bug that makes GitQlient to crash
- Bug: Bug that makes GitQlient unstable. It doesn't crash
- Documentation: The issue is only about documentation
- Ready to review: The issue is ready to review

As soon as I see need of more I'll add them.


### About reporting issues

If you want to report a bug, please make sure to verify that it exists in the latest commit of master or in the current version.

## Code contributions
If you want to implement a new feature or solve bugs in the Issues page, you can pick it up there and start coding!

If you're familiar with Qt and/or C++, you can go directly to the [features](https://github.com/francescmm/GitQlient/labels/Feature) or the [bugs](https://github.com/francescmm/GitQlient/labels/Bug). Otherwise, the [rookie tasks](https://github.com/francescmm/GitQlient/labels/Rookie%20task) are a nice way to start.

### How to contribute with code

First of all, you must [fork GitQlient](https://help.github.com/en/github/getting-started-with-github/fork-a-repo) and clone into your computer. In addition, you must have configured the original GitQlient repository to upstream:

```git remote add upstream https://github.com/Stellarium/stellarium.git```

When you start with an issue, make sure you create a branch from master:

```git checkout master
git pull upstream master
git checkout -b nameOfMyBranch
```

Ideally, your branch name should have the following schema:

- feature/short-feature-title
- bug/short-bug-title
- critical/short-critical-bug-title

Following these formats makes it a lot easier to know what you want to achieve and who is the responsible and aid us in getting contributions integrated as quickly as possible. Remember to follow the Code styles and the Code guidelines.

Once you are done with your changes and you have pushed them to your branch, you can create a [Pull Request](https://github.com/francescmm/GitQlient/pulls). Remember to add a good title and description. And don't forget to add the label!

### Code styles

GitQlient follows the [Qt Code Style](https://wiki.qt.io/Qt_Coding_Style) as well as the [Coding Conventions](https://wiki.qt.io/Qt_Coding_Style) when they are not agains the C++ Core Guidelines. In fact, there is a .clang-format file in the GitQlient repository you can use to format your code.

I don't mind that you have your own style when coding: it's easier to do it with your own style! However, before the code goes to the Pull Request you should format it so it looks as the code in the repo.

### Code guidelines

Some time ago, [Bjarne Stroustrup](http://www.stroustrup.com) and [Herb Sutter](http://herbsutter.com/) started an amazing project called [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). I know that is a large document and I don't expect that anybody reads the whole thing or memorizes it. I won't.

But in case of doubt, is the place where we should look on how to do things or why some things are done in the way they are. Having this kind of guidelines is the best way to avoid gut feelings regarding code.
