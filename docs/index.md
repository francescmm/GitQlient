---
layout: default
---

# GitQlient User Manual

## Introduction

GitQlient, pronounced as git+client (/gɪtˈklaɪənt/) is a multi-platform Git client originally forked from QGit. Nowadays it goes beyond of just a fork and adds a lot of new functionality.

The original idea was to provide a GUI-oriented Git client that was easy to integrate with QtCreator (currently shipped as GitQlientPugin). This idea has grown since the day 1 to not only cover the integration with QtCreator but also to make it an app on it's own.

The primarily idea behind GitQlient was to modernize the old code that QGit is based on and provide a easy UI/UX that I was actually missing on other clients. That was achieved in the version 1.0.0 of GitQlientPlugin in December. On that version, some features that were not part of QGit were included to make it easier to handle the Git repository.

After that, I felt free to open the gates for the big new features that I was actually missing in some of the Git clients I was using. Some of the features are meant to be application-based but other's are related with the spirit of the open source.

But, what that means exactly?

There are several things, but I feel that two in particular are missed most of the time. The first one is a good documentation. I'm not talking only about adding Doxygen documentation to the header files or the APIs. I mean also to provide documentation about **how** is designed the application and **why** I did it in that way. I'd like that this technical documentation helps whoever wants to contribute to the project.

The second thing I've been missing is a proper User Manual. After several years in the software industry, I'm aware that one thing is writing the code of an application and another completely different is to be able to explain how to use it. When I write the code of a project or a feature, I don't need to write a User Manual to know how it works; I've created it!

But, what happens if you want to introduce as much people as possible? Well, then we need a User Manual that tells them exactly how things work. What options they have and how to deal with possible mistakes or errors.

This document tries to cover exactly this area.

## Glossary

Here you can find all the specific glossary that will be used in this document referring to GitQlient:

- **WIP**: Work in progress. Usually refers to the local modification in files of the repository that are not committed yet.

# How to use GitQlient

## Initial screen

## GitQlient configuration

## Initializing a new repository

## Cloning a remote repository

## Open an existing repository

# Quick access actions

# Main repository view

## The repository graph tree

### Commit selection

#### Options

### Contextual menu options

#### Options for the WIP

#### Options for the last commit

#### Options other commits

## Viewing the changes of a commit

### WIP view

### Commit info view

## Branches information panel

### Local branches

### Remote branches

### Stashes

### Submodules

# Diff view

## Commit diff view

## File diff view

# Blame & History view

## The file view

## The blame view

## The history view

# Merge view

# Appendix A: Releases

# Appendix B: Development

## Set up & Build the code

### Pre-requirements

### How to build

### How to contribute

# Appendix C: Recognition

# Appendin D: License