import mydb
import sys

G = {
    "income" : ["bmi", "exercise", "smoke", "cholesterol", "bp"],
    "bmi" : ["diabetes", "stroke", "attack", "angina"],
    "exercise" : ["bmi", "bp", "cholesterol"],
    "bp" : ["stroke", "attack", "angina"],
    "cholesterol" : ["stroke", "attack", "angina"],
    "smoke": ["bp", "cholesterol"],
    "diabetes" : [],
    "stroke" : [],
    "attack" : [],
    "angina" : []

}

Options = {
    "income" : 4,
    "bmi" : 4,
    "default" : 2
}

G_ = {}

CPTS = {}

def reverse_tree():
    for key in G:
        G_[key] = []
        for key2 in G:
            if key in G[key2]:
                G_[key].append(key2)

def build_cpt(node):
    columns = G_[node][:]
    possible = 2
    if node in Options:
        possible = Options[node]
    for i in range(possible):
        columns.append("p"+str(i))
    mydb.create_table("cpt_"+node, columns)
    for perm in permutation(G_[node]):
        rfcolumns = G_[node][:]
        rfcolumns.append(node)
        p1 = mydb.count("riskfactor", G_[node], perm)
        possible = 2
        if node in Options:
            possible = Options[node]
        row = perm[:]
        for i in range(possible): 
            rfrow = perm[:]
            rfrow.append(i)
            p2 = mydb.count("riskfactor", rfcolumns, rfrow)
            row.append(round(float(p2)/p1,4))
        mydb.insert("cpt_"+node, row)

def build_cpts():
    for node in G:
        build_cpt(node)

def permutation(plist):
    if len(plist) == 0:
        return [[]]
    l = []
    opts = 2
    clist = plist[1:]
    perm = permutation(clist)
    for i in perm:
        if plist[0] in Options:
            opts = Options[plist[0]]
        for j in range(opts):
            ll = [j]
            ll.extend(i)
            l.append(ll)
    return l

def parse_line(line):
    data = line.split()
    values = []
    for i in range(len(data)):
        if i == 0:
            income = int(data[i])
            if income <= 25000:
                value = 0 
            elif income <= 50000:
                value = 1 
            elif income <= 75000:
                value = 2 
            else:
                value = 3
        elif i == 3:
            if data[i] == "underweight":
                value = 0 
            elif data[i] == "normal":
                value = 1
            elif data[i] == "overweight":
                value = 2
            else:
                value = 3
        else:
            value = 1 if data[i] == "yes" else 0
        values.append(value)
    return values

def prepare_data(datafile):
    columns = datafile.readline().split()
    mydb.create_table("riskfactor", columns)
    for line in datafile:
        mydb.insert("riskfactor", parse_line(line))

"""
def prob_l2h(target, given):
    if len(target) == 1:
        key = target.keys()[0]
        columns = []
        values = []
        for parent in G_[key]:
            if parent not in given:
                p = 0
                possible = 2
                if parent in Options:
                    possible = Options[parent]
                for i in range(possible):
                    add = given.copy()
                    add[parent] = i
                    p1 = prob_l2h(target, add)
                    p2 = prob_l2h({parent : i}, given)
                    p += p1 * p2
                return p
            else:
                columns.append(parent)
                values.append(given[parent])
        return mydb.get("cpt_"+key, columns, values, "p"+str(target[key]))
    lowest = None
    for key in target:
        if lowest == None:
            lowest = key
        elif lower(key, lowest):
            lowest = key
    newgiven = given.copy()
    for key in target:
        if key != lowest:
            newgiven[key] = target[key]
    newtarget = target.copy()
    del newtarget[lowest]
    final = prob_l2h({lowest : target[lowest]}, newgiven) \
            * prob_l2h(newtarget, given)
    return final

def prob(target, given):
    sep = False
    for key1 in target:
        for key2 in given:
            if not lower(key1, key2):
                sep = True
                break
    if sep:
        target1 = {}
        target2 = {}
        given1 = {}
        given2 = {}
        for key in target:
            target1[key] = target[key]
        for key in given:
            target1[key] = given[key]
            target2[key] = given[key]
        return prob_l2h(target1, given1) / prob_l2h(target2, given2)
    else:
        return prob_l2h(target, given)
"""

def enum_ask(target, given):
    hidden = []
    Qx = {}
    X = []
    Goal = []
    Vars = []
    for node in G:
        Vars.append(node)
    for key in target:
        X.append(key)
        Goal.append(target[key])
    for assign in permutation(X):
        xgiven = given.copy()
        for i in range(len(assign)):
            xgiven[X[i]] = assign[i]
        Qx[str(assign)] = enum_all(Vars, xgiven)
    total = 0
    for assign in Qx:
        total += Qx[assign]
    return Qx[str(Goal)] / total

def enum_all(Vars, xgiven):
    if len(Vars) == 0:
        return 1
    varscopy = Vars[:]
    for i in varscopy:
        highest = True
        for j in varscopy:
            if j == i:
                continue
            if not lower(j, i):
                highest = False
        if highest:
            Y = i
            break
    varscopy.remove(Y)
    columns = []
    values = []
    for parent in G_[Y]:
        columns.append(parent)
        values.append(xgiven[parent])
    if Y in xgiven:
        p1 = mydb.get("cpt_" + Y, columns, values, "p" + str(xgiven[Y]))
        p2 = enum_all(varscopy, xgiven)
        return p1 * p2
    else:
        possible = 2
        p = 0
        if Y in Options:
            possible = Options[Y]
        for value in range(possible):
            givencopy = xgiven.copy()
            givencopy[Y] = value
            p1 = mydb.get("cpt_" + Y, columns, values, "p" + str(value))
            p2 = enum_all(varscopy, givencopy)
            p += p1*p2
        return p
            

def lower(node1, node2):
    if len(G[node1]) == 0:
        return True
    if node2 in G[node1]:
        return False
    for child in G[node1]:
        if not lower(child, node2):
            return False
    return True

def int_var(dic):
    for key in dic:
        if key == "income":
            if dic[key] <= 25000:
                dic[key] = 0 
            elif dic[key] <= 50000:
                dic[key] = 1 
            elif dic[key] <= 75000:
                dic[key] = 2 
            else:
                dic[key] = 3
        elif key == "bmi":
            if dic[key] == "underweight":
                dic[key] = 0 
            elif dic[key] == "normal":
                dic[key] = 1
            elif dic[key] == "overweight":
                dic[key] = 2
            else:
                dic[key] = 3
        else:
            dic[key] = 1 if dic[key] == "yes" else 0

def main():
    if len(sys.argv) != 5:
        print "Wrong arg num"
        return False
    if sys.argv[1] != "-i":
        print "Wrong arg"
        return False
    if sys.argv[3] != "-d":
        print "Wrong arg"
        return False
    inputfile_name = sys.argv[2]
    datafile_name = sys.argv[4]
    datafile = open(datafile_name, "r")
    prepare_data(datafile)
    reverse_tree()
    build_cpts()
    inputfile = open(inputfile_name, "r")
    outputfile = open("riskFactor.out", "w")
    casen = int(inputfile.readline())
    for line in inputfile:
        l = eval(line)
        target = l[0]
        int_var(target)
        given = l[1]
        int_var(given)
        p = "%.4f" % round(enum_ask(target, given),4)
        print p
        outputfile.write(p+"\n")


main()
