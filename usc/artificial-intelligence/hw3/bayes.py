import sys


def sol(diseases, values):
    q1 = {}
    q2 = {}
    q3 = {}
    for i in range(len(diseases)):
        p1 = 1
        p2 = 1
        minp = 1
        maxp = 1
        incr = [1, "T", ""]
        decr = [0, "F", ""]
        for j in range(diseases[i]["fnum"]):
            if values[i][j] == 'T':
                p1 *= diseases[i]["ppresent"][j]
                p2 *= diseases[i]["punpresent"][j]
            elif values[i][j] == 'F':
                p1 *= 1 - diseases[i]["ppresent"][j]
                p2 *= 1 - diseases[i]["punpresent"][j]
            else:
                pt = diseases[i]["punpresent"][j] / diseases[i]["ppresent"][j]
                pf = (1 - diseases[i]["punpresent"][j]) / (1 - diseases[i]["ppresent"][j])
                if max(pt, pf) > decr[0]:
                    decr[0] = max(pt, pf)
                    decr[1] = "T" if decr[0] == pt else "F"
                    decr[2] = diseases[i]["findings"][j]
                if min(pt, pf) < incr[0]:
                    incr[0] = min(pt, pf)
                    incr[1] = "T" if incr[0] == pt else "F"
                    incr[2] = diseases[i]["findings"][j]
                maxp *= min(pt, pf)
                minp *= max(pt, pf)
        p = 1 / (1 + p2 * (1 - diseases[i]["p"]) / (p1 * diseases[i]["p"]))
        pmax = 1 / (1 + p2 * (1 - diseases[i]["p"]) / (p1 * diseases[i]["p"]) * maxp)
        pmin = 1 / (1 + p2 * (1 - diseases[i]["p"]) / (p1 * diseases[i]["p"]) * minp)
        q1[diseases[i]["name"]] = str("%.4f" % round(p, 4))
        q2[diseases[i]["name"]] = []
        q2[diseases[i]["name"]].append(str("%.4f" % round(pmin, 4)))
        q2[diseases[i]["name"]].append(str("%.4f" % round(pmax, 4)))
        q3[diseases[i]["name"]] = []
        q3[diseases[i]["name"]].append(incr[2])
        q3[diseases[i]["name"]].append(incr[1])
        q3[diseases[i]["name"]].append(decr[2])
        q3[diseases[i]["name"]].append(decr[1])
    return q1, q2, q3


def main():
    if len(sys.argv) != 3:
        print "Wrong arg num"
        return False
    if sys.argv[1] != "-i":
        print "Wrong arg"
        return False
    filename = sys.argv[2]
    results = open(filename.split(".")[0] + "_inference.txt", "w")
    f = open(filename, "r")
    dnum, pnum = f.readline().split()
    dnum = int(dnum)
    pnum = int(pnum)
    diseases = {}
    for i in range(dnum):
        diseases[i] = {}
        diseases[i]["name"], diseases[i]["fnum"], diseases[i]["p"] = f.readline().split()
        diseases[i]["fnum"] = int(diseases[i]["fnum"])
        diseases[i]["p"] = float(diseases[i]["p"])
        diseases[i]["findings"] = eval(f.readline())
        diseases[i]["ppresent"] = eval(f.readline())
        diseases[i]["punpresent"] = eval(f.readline())
    for i in range(pnum):
        values = []
        for j in range(dnum):
            values.append(eval(f.readline()))
        q1, q2, q3 = sol(diseases, values)
        results.write("Patient-%i:\n" % (i + 1))
        results.write(str(q1) + "\n")
        results.write(str(q2) + "\n")
        results.write(str(q3) + "\n")

main()
