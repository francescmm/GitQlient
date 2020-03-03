# Setup & Build

## Pre-requisites

### Qt

GitQlient releases are built with the latest version of Qt, currently [Qt 5.14](https://www.qt.io/download-qt-installer). However, any version above the last LTS (Qt 5.12) is guaranteed to compile.

The plan for the near future is to test for every major version from Qt 5.12 to the latest.

### Git

GitQlient is not using any library to access Git commands. For that reason, you will need to have Git installed and added to the path.

## Submodules

For now, GitQlient has only one dependency and that's [QLogger](https://github.com/francescmm/QLogger) what it's just two files!

Remember that the first time you will need to initialize the submodule and update it from time to time.

## Steps

If you want to play a bit with GitQlient or just build it for your own environment, you will have to do:

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

5. Note: By default ```make install``` puts the executable in ```$HOME/GitQlient/bin```.
