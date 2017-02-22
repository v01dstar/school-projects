import sys


NOTDECIDED = -1


def valueOf(clause, model):
    if isLiteral(clause):
        not_symbol = True
        if isVariable(clause):
            not_symbol = False
            variable = clause
        else:
            variable = clause[1]
        if variable in model:
            if not_symbol:
                return 1 - model[variable]
            else:
                return model[variable]
        else:
            return NOTDECIDED
    else:
        result = False
        for literal in clause:
            value = valueOf(literal, model)
            if value == 1:
                return True
            if value == NOTDECIDED:
                result = NOTDECIDED
        return result

def isLiteral(clause):
    if type(clause) == list and len(clause) == 2 and clause[0] == "not" and isVariable(clause[1]):
        return True
    if isVariable(clause):
        return True
    return False

def isVariable(elem):
    if type(elem) == str and len(elem) == 1 and elem.isalpha():
        return True
    return False

def DPLL(clauses, symbols, model):
    flag = True
    for clause in clauses:
        value = valueOf(clause, model)
        if value == 0:
            return False
        if value == NOTDECIDED:
            flag = NOTDECIDED
    if flag == True:
        return True
    variable, value = find_pure_symbols(symbols, clauses, model)
    if variable != None:
        symbols.remove(variable)
        model[variable] = value
        return DPLL(clauses, symbols, model)
    variable, value = find_unit_clause(clauses, model)
    if variable != None:
        symbols.remove(variable)
        model[variable] = value
        return DPLL(clauses, symbols, model) 
    return split_rule(clauses, symbols, model)
    vaule2 = DPLL(clauses, symbols, model)
  
def split_rule(clauses, symbols, model):
    symbol = symbols[0]
    del symbols[0]
    if guess(clauses, symbols, model, symbol, 1):
        return True
    if guess(clauses, symbols, model, symbol, 0):
        return True
    return False

def guess(clauses, symbols, model, symbol, value):
    symbols_backup = symbols[:]
    model_backup = model.copy()
    model_backup[symbol] = value
    result = DPLL(clauses, symbols_backup, model_backup)
    if result == True:
        assigned_symbols = []
        for symbol in symbols:
            if symbol not in symbols_backup:
                assigned_symbols.append(symbol)
        for symbol in assigned_symbols:
            symbols.remove(symbol)
        for key in model_backup:
            if key not in model:
                model[key] = model_backup[key]
        return True
    return False

def find_pure_symbols(symbols, clauses, model):
    hashmap = {}
    for clause in clauses:
        if valueOf(clause, model) == 1:
            continue
        if isLiteral(clause):
            if type(clause) == list:
                hash(hashmap, clause[1], 0)
            else:
                hash(hashmap, clause, 1)
        else:
            for literal in clause:
                if type(literal) == list:
                    hash(hashmap, literal[1], 0)
                else:
                    hash(hashmap, literal, 1)
    for key in hashmap:
        if hashmap[key] != -1 and key not in model:
            return key, hashmap[key]
    return None, None


def hash(hashmap, key, value):
    if key in hashmap:
        if hashmap[key] != value:
            hashmap[key] = -1
    else:
        hashmap[key] = value

def find_unit_clause(clauses, model):
    for clause in clauses:
        if isLiteral(clause):
            if type(clause) == list:
                variable = clause[1]
                value = 0
            else:
                variable = clause
                value = 1
            if variable not in model:
                return variable, value
        else:
            variable = None
            value = None
            if valueOf(clause, model) == NOTDECIDED:
                for literal in clause:
                    if type(clause) == list:
                        if not variable:
                            variable = clause[1]
                            value = 0
                        else:
                            break
                    else:
                        if not variable:
                            variable = clause
                            value = 1
                        else:
                            break
    return None, None

def appendClause(clauses, clause, symbols):
    if isLiteral(clause):
        clauses.append(clause)
        if isVariable(clause):
            if clause not in symbols:
                symbols.append(clause)
        elif clause[1] not in symbols:
            symbols.append(clause[1])
    else:
        clauses.append(clause[1:])
        for literal in clause[1:]:
            if isVariable(literal):
                if literal not in symbols:
                    symbols.append(literal)
            elif literal[1] not in symbols:
                symbols.append(literal[1])


def main():
    if len(sys.argv) != 3:
        print "Wrong arg num"
        return False
    if sys.argv[1] != "-i":
        print "Wrong arg"
        return False
    filename = sys.argv[2]
    results = open("CNF_satisfiability.txt", "wr")
    f = open(filename, "r")
    n = eval(f.readline())
    for i in range(n):
        l = eval(f.readline())
        print l
        clauses = []
        symbols = []
        model = {}
        if isLiteral(l):
            appendClause(clauses, l, symbols)
        else:
            if l[0] == "or":
                appendClause(clauses, l[1:], symbols)
            else:
                for clause in l[1:]:
                    appendClause(clauses, clause, symbols)
        value = DPLL(clauses, symbols, model)
        result = []
        if value == 1:
            result.append("true")
        if value == 0:
            result.append("false")
        if value:
            for key in model:
                if model[key] == 1: 
                    result.append(key+"=true")
                else:
                    result.append(key+"=false")
            for elem in symbols:
                result.append(elem+"=true")
        print result
        results.write(str(result)+"\n")


main()
