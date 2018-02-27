Quassel IRC [![Linux Build Status](https://travis-ci.org/quassel/quassel.svg?branch=master)](https://travis-ci.org/quassel/quassel) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/quassel/quassel?branch=master&svg=true)](https://ci.appveyor.com/project/quassel/quassel/branch/master)
===============

[Quassel IRC][web-home] is a modern, cross-platform, distributed IRC client,
meaning that one (or multiple) client(s) can attach to and detach from a
central core -- much like the popular combination of screen and a text-based
IRC client such as WeeChat, but graphical.

Not only do we aim to bring a pleasurable, comfortable chatting experience to
all major platforms, but it's free - as in beer and as in speech, since we
distribute Quassel under [the GPL](https://www.gnu.org/licenses/gpl.html), and
you are welcome to download and see for yourself!

## Release notes
You can find the current release notes on the [Quassel IRC homepage][web-home],
as well as in this repository's [```ChangeLog```][repo-changelog].

## Quick reference

On first run of the Quassel core, it will wait for a client to connect
and present a first-run wizard that will allow you to create the database
and one admin user for the core-side storage.

* To add more users, run: ```quasselcore --add-user```
* To change the password of an existing user: ```quasselcore --change-userpass=username```

On some systems, you may need to specify ```--configdir```, e.g.
```quasselcore --configdir=/var/lib/quassel [command]```.

The ```manageusers.py``` script is deprecated and should no longer be used.

To learn more, see [the Quassel project wiki][docs-wiki] for in-depth
documentation.

## Getting involved

IRC is the preferred means of getting in touch with the developers.

The Quassel IRC Team can be contacted on **```Freenode/#quassel```**
(or **```#quassel.de```**).  If you have trouble getting Quassel to connect,
you can use [Freenode's webchat][help-freenode].

We always welcome new users in our channels!

You can learn more and reach out to us in several ways:
* [Visit our homepage][web-home]
* [Submit and browse issues on the bugtracker][dev-bugs]
 * Github issues are not used, but [pull requests][dev-pr-new] are accepted!
* [Email the dev team - devel@quassel-irc.org][dev-email]

Thanks for reading,

~ *The Quassel IRC Team*

[web-home]: https://quassel-irc.org
[dev-bugs]: https://bugs.quassel-irc.org
[dev-email]: mailto:devel@quassel-irc.org
[dev-pr-new]: https://github.com/quassel/quassel/pull/new/master
[docs-wiki]: https://bugs.quassel-irc.org/projects/quassel-irc/wiki
[help-freenode]: https://webchat.freenode.net?channels=%23quassel
[repo-changelog]: ChangeLog
