/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/** This is called at compile time and generates a suitable version.gen.
 *  usage: genversion git_root target_file
 * 
 */

#include <QDebug>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QFile>

int main(int argc, char **argv) {
  if(argc < 3) {
    qFatal("Usage: ./genversion <git_root> <target_file>");
    return 255;
  }
  QString gitroot = argv[1];
  QString target = argv[2];
  QString version, commit, archivetime;

  if(QFile::exists(gitroot + "/.git")) {
    // try to execute git-describe to get a version string
    QProcess git;
    git.setWorkingDirectory(gitroot);
    git.start("git", QStringList() << "describe" << "--long");
    if(git.waitForFinished(10000)) {
      QString gitversion = git.readAllStandardOutput();
      if(!gitversion.isEmpty() && !gitversion.contains("fatal")) {
        // seems we have a valid version string, now prettify it...
        // check if the workdir is dirty first
        QString dirty;
        QStringList params = QStringList() << "diff-index" << "--name-only" << "HEAD";
        git.start("git", params);
        if(git.waitForFinished(10000)) {
          if(!git.readAllStandardOutput().isEmpty()) dirty = "*";
        }
        // Now we do some replacement magic...
        QRegExp rxCheckTag("(.*)-0-g[0-9a-f]+\n$");
        QRegExp rxGittify("(.*)-(\\d+)-g([0-9a-f]+)\n$");
        gitversion.replace(rxCheckTag, QString("\\1%1").arg(dirty));
        gitversion.replace(rxGittify, QString("\\1:git-\\3+\\2%1").arg(dirty));
        if(!gitversion.isEmpty()) version = gitversion;
      }
    }
  }
  if(version.isEmpty()) {
    // hmm, Git failed... let's check for version.dist instead
    QFile dist(gitroot + "/version.dist");
    if(dist.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QRegExp rxCommit("(^[0-9a-f]+)");
      QRegExp rxTimestamp("(^[0-9]+)");
      if(rxCommit.indexIn(dist.readLine()) > -1) commit = rxCommit.cap(1);
      if(rxTimestamp.indexIn(dist.readLine()) > -1) archivetime = rxTimestamp.cap(1);
      dist.close();
    }
  }
  // ok, create our version.gen now
  QFile gen(target);
  if(!gen.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qFatal("%s", qPrintable(QString("Could not write %1!").arg(target)));
    return 255;
  }
  gen.write(QString("quasselGeneratedVersion = \"%1\";\n"
                    "quasselBuildDate = \"%2\";\n"
                    "quasselBuildTime = \"%3\";\n"
                    "quasselCommit = \"%4\";\n"
                    "quasselArchiveDate = %5;\n")
                    .arg(version).arg(__DATE__).arg(__TIME__).arg(commit).arg(archivetime.toUInt()).toAscii());
  gen.close();
  return EXIT_SUCCESS;
}
