Quassel IRC
===============

[![Quassel CI Build Status][ci-badge]][ci-status-page]

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
as well as in this repository's [`ChangeLog`][repo-changelog].

## Downloading
Official, stable downloads are provided on the [Quassel IRC download page](https://quassel-irc.org/downloads).

Automated Windows and macOS builds are available via the [GitHub Actions tab][ci-status-page].  Pick a build, then download the appropriate artifact.

*NOTE: As of this writing, you need to log into GitHub to download the automated builds.  This [issue is being tracked with GitHub](https://github.com/actions/upload-artifact/issues/51 ).*

Unofficial builds and testing versions are [contributed by several community members](https://bugs.quassel-irc.org/projects/quassel-irc/wiki#Unofficial-builds).

## Quick reference
We recommend reading the [getting started guide on the wiki](https://bugs.quassel-irc.org/projects/quassel-irc/wiki#Getting-started),
but in a pinch, these steps will do.

On first run of the Quassel core, it will wait for a client to connect
and present a wizard that will allow you to create the database and one admin
user for the core-side storage.

Once you've set up Quassel, you may:
* Add more users: `quasselcore --add-user`
* Change the password of an existing user: `quasselcore --change-userpass=username`
* See all available options: `quasselcore --help`

On some systems, you may need to specify `--configdir`, e.g.
`quasselcore --configdir=/var/lib/quassel [command]`.

To learn more, see [the Quassel project wiki](https://bugs.quassel-irc.org/projects/quassel-irc/wiki) for in-depth
documentation.

## Getting involved

IRC is the preferred means of getting in touch with the developers.

The Quassel IRC Team can be contacted on **`Freenode/#quassel`**
(or **`#quassel.de`**).  If you have trouble getting Quassel to connect,
you can use [Freenode's webchat][help-freenode].

We always welcome new users in our channels!

You can learn more and reach out to us in several ways:
* [Visit our homepage][web-home]
* [Submit and browse issues on the bugtracker](https://bugs.quassel-irc.org)
  * GitHub issues are not used, but [pull requests](https://github.com/quassel/quassel/pull/new/master) are accepted!
* [Email the dev team - devel@quassel-irc.org][dev-email]

Thanks for reading,

~ *The Quassel IRC Team*

[web-home]: https://quassel-irc.org
[dev-email]: mailto:devel@quassel-irc.org
[help-freenode]: https://webchat.freenode.net?channels=%23quassel
[repo-changelog]: ChangeLog
[ci-badge]: https://github.com/quassel/quassel/workflows/Quassel%20CI/badge.svg?branch=master
[ci-status-page]: https://github.com/quassel/quassel/actions?query=branch%3Amaster
