#!/usr/bin/python
# -*- coding: iso-8859-1 -*-

# ==============================
#  Imports
# ==============================
import os
import sha
import sys

try:
    import sqlite3
except ImportError:
    print >> sys.stderr, "ERROR: sqlite3 module not available!"
    print >> sys.stderr, "This script needs sqlite3 support which is part of Python 2.5"
    print >> sys.stderr, "You probably need to upgrade your Python installation first."
    sys.exit(3)

class UserManager(object):
    def __init__(self):
        self.db = sqlite3.connect(os.environ['HOME'] + '/.quassel/quassel-storage.sqlite')
        
    def __del__(self):
        self.db.commit()
        self.db.close();

    def shaCrypt(self, password):
        shaPass = sha.new(password)
        return shaPass.hexdigest()
        
    def addUser(self, username, password):
        cursor = self.db.cursor()
        cursor.execute('INSERT INTO quasseluser (username, password) VALUES (:username, :password)',
                       {'username':username, 'password':self.shaCrypt(password)})

    def changePass(self, username, password):
        cursor = self.db.cursor()
        cursor.execute('UPDATE quasseluser SET password = :password WHERE username = :username',
                       {'username':username, 'password':self.shaCrypt(password)})

if __name__ == "__main__":
    generalError = "ERROR: Wrong argument count (Syntax: %s add|changepass <username> <password>)" % sys.argv[0]
    if len(sys.argv) < 3:
        print generalError
        sys.exit(1)

    if sys.argv[1].lower() not in ['add', 'changepass']:
        print generalError
        sys.exit(2)

    userManager = UserManager()
    actions = {'add':userManager.addUser,
               'changepass':userManager.changePass}

    actions[sys.argv[1]](sys.argv[2], sys.argv[3])
    
