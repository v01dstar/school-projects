"""
import sqlite3

con = sqlite3.connect("hw3.db")
bunch_con = sqlite3.connect("hw3.db")

def create_table(tablename, columns):
    sql = "CREATE TABLE " + tablename + "( UUID INTEGER PRIMARY KEY"
    for column in columns:
        sql += ", " + column + " INT"
    sql += ");"
    print sql
    with con:
        cur = con.cursor()
        try:
            cur.execute("DROP TABLE " + tablename + ";")
            print "Already exists, drop", tablename
        except:
            print "Creating table ..."
        cur.execute(sql)

def count(tablename, columns, values):
    sql = "SELECT COUNT(*) FROM " + tablename
    if len(columns) > 0:
        sql += " WHERE "
    for i in range(len(columns)):
        if i != 0:
            sql += " AND "
        sql += columns[i] + " = " + str(values[i])
    sql += ";"
    with con:
        cur = con.cursor()
        cur.execute(sql)
        row = cur.fetchone()
    return row[0]

def insert(tablename, columns, values):
    sql = "INSERT INTO " + tablename + " ("
    for i in range(len(columns)):
        if i != 0:
            sql += ", "
        sql += columns[i]
    sql += ") VALUES ("
    for i in range(len(values)):
        if i != 0:
            sql += ", "
        sql += str(values[i])
    sql += ");"
    with con:
        cur = con.cursor()
        cur.execute(sql)

def bunch_insert(tablename, columns, values):
    sql = "INSERT INTO " + tablename + " ("
    for i in range(len(columns)):
        if i != 0:
            sql += ", "
        sql += columns[i]
    sql += ") VALUES ("
    for i in range(len(values)):
        if i != 0:
            sql += ", "
        sql += str(values[i])
    sql += ");"
    cur = bunch_con.cursor()
    cur.execute(sql)

def get(tablename, columns, values, target):
    sql = "SELECT " + target + " FROM " + tablename
    if len(columns) > 0:
        sql += " WHERE "
    for i in range(len(columns)):
        if i != 0:
            sql += " AND "
        sql += columns[i] + " = " + str(values[i])
    sql += ";"
    with con:
        cur = con.cursor()
        cur.execute(sql)
        row = cur.fetchone()
    return row[0]

def test():
    with con:
        cur = con.cursor()
        cur.execute("SELECT * FROM riskfactor WHERE Income = 1;")
        row = cur.fetchone()
    print "Return" + str(row)


def main():
    f = open("Risk_Factor_data.txt", "r")
    prepare_data(f)
    test()
"""
db = {}

def create_table(tablename, columns):
    db[tablename] = {}
    db[tablename]["columns"] = columns
    db[tablename]["rows"] = []

def insert(tablename, values):
    db[tablename]["rows"].append(values)

def count(tablename, columns, values):
    count = 0
    indexs = [] 
    for i in range(len(columns)):
        column = columns[i]
        index = db[tablename]["columns"].index(column)
        indexs.append(index)
    for row in db[tablename]["rows"]:
        i = 0
        hit = True
        for index in indexs:
            if row[index] != values[i]:
                hit = False
                break
            i += 1
        if hit == True:
            count += 1
    return count

def get(tablename, columns, values, target):
    indexs = []
    for i in range(len(columns)):
        column = columns[i]
        index = db[tablename]["columns"].index(column)
        indexs.append(index)
    for row in db[tablename]["rows"]:
        i = 0
        hit = True
        for index in indexs:
            if row[index] != values[i]:
                hit = False
                break
            i += 1
        if hit == True:
            return row[db[tablename]["columns"].index(target)]
    return None
