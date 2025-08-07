VYM - View Your Mind (c) 2004-2025 by Uwe Drechsel
==================================================

About
-----

VYM is a mind mapping application. You can use it to visualize your 
thoughts in tree-like structures. It is also useful for time management, 
self-organization and sorting through new ideas and complex contexts.

VYM includes a powerful personal task manager, which allows to adjust
priorities or reminds you after easily defined time spans.

You can use the scripting capabilities for interesting presentations. 
VYM can also retrieve data from the Jira issue tracking system and 
talk to the Confluence documentation system.

VYM runs on Apple and Windows and of course most Linux platforms.

Documentation
-------------

* PDF

    The complete documentation of vym is available as PDF document in
    English, French and Spanish. (Only the English version is really up
    to date, translators are welcome). PDFs can be accessed directly from
    vym via the help section. They also can be downloaded from github at

    https://github.com/insilmaril/vym/tree/develop/doc

* Release notes and changelog

    Can either be downloaded directly in vym via
    "Help - Download and show release notes" or can be accessed in
    https://github.com/insilmaril/vym/blob/release/release-notes-2.9.md

* Screencasts

    There are several screencasts available on YouTube, which show some
    features of vym and how to use them:

    http://www.youtube.com/user/ViewYourMind


Download
--------

The official downloads for all platforms are available at the project
site:

https://sourceforge.net/projects/vym/

The latest development drops for all platforms can be found there in the
folder Files->Development.

The latest binaries for Linux, Debian and Ubuntu are built and available
in the authors Open Build Service project:

https://software.opensuse.org/download.html?project=home%3Ainsilmaril&package=vym


Source code
-----------

The source code was hosted on Sourceforge for 16 years, but in
2021 the development moved to github:

https://github.com/insilmaril/vym


Installation
------------

* Binaries

    Binaries for all major platforms can be found on:

    https://sourceforge.net/projects/vym/

    Packages there are: openSUSE rpm, Ubuntu deb, Apple dmg, Windows exe

    More Linux like packages and developer versions are built in
    Open Build Service:

    https://build.opensuse.org/package/show/home:insilmaril/vym


* Compiling

    Compiling vym from scratch is pretty easy, if you have the
    development packages of the Qt6 toolkit installed. (Check also the
    homepage above for details):

    On the command line you can

      cmake .
      make
      make install

    or using Qt Creator (recommended on Mac and Windows):

    In "File" do "Open file or project" and select the
    "CMakeLists.txt". This will setup the project.

    
    For testing you probably need to tell vym where to find various
    files like macros, demos, etc. On the commandline you can do this
    with the "-l" option. Using Qt Creator you can add a variable
    VYMHOME to the execution environment pointing to your path to vym
    sources.


Questions and feedback
----------------------

Bugs and feature requests will be taken care of in

  https://github.com/insilmaril/vym/issues

Please direct support questions to the mailinglist first:

  vym-forum@lists.sourceforge.net
