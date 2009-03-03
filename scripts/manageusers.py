#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Requires at least python version 2.5

"""Usage:
    manageusers.py (add|changepass) <username> <password>
    manageusers.py list"""

import os
import sha
import sys
from pprint import pprint

try:
    import sqlite3
except ImportError:
    print >> sys.stderr, "ERROR: sqlite3 module not available!"
    print >> sys.stderr, "This script needs sqlite3 support which is part of Python 2.5"
    print >> sys.stderr, "You probably need to upgrade your Python installation first."
    sys.exit(3)

class UserManager(object):
    def __init__(self):
        dbpaths = [os.environ['HOME'] + '/.quassel/quassel-storage.sqlite',
                   os.environ['HOME'] + '/.config/quassel-irc.org/quassel-storage.sqlite',
                   '/var/cache/quassel/quassel-storage.sqlite']
        for dbpath in dbpaths:
            if os.path.exists(dbpath):
                self.db = sqlite3.connect(dbpath)
                break

        self.cursor = self.db.cursor()

    def __del__(self):
        self.db.commit()
        self.db.close();

    def _shaCrypt(self, password):
        return sha.new(password).hexdigest()

    def add(self, username, password):
        self.cursor.execute('INSERT INTO quasseluser (username, password) VALUES (:username, :password)',
            {'username':username, 'password':self._shaCrypt(password)}).fetchone()

    def changepass(self, username, password):
        self.cursor.execute('UPDATE quasseluser SET password = :password WHERE username = :username',
            {'username':username, 'password':self._shaCrypt(password)}).fetchone()

    def list(self):
        return self.cursor.execute("SELECT * FROM quasseluser").fetchall()

    def callByName(self, name, *args, **kws):
        return self.__getattribute__(name)(*args, **kws)

def main():
    usermanager = UserManager()

    try:
        action = sys.argv[1].lower()
    except:
        print(__doc__)
        return

    if action == 'list':
        pprint(usermanager.list())
    elif action in ['add', 'changepass'] and len(sys.argv) > 3:
        usermanager.callByName(action, sys.argv[2], sys.argv[3])
    else:
        print("ERROR: Wrong arguments supplied.")
        print(__doc__)

if __name__ == "__main__":
    main()
