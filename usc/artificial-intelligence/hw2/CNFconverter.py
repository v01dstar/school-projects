import sys

def isLiteral(elem):
    if type(elem) == list and len(elem) == 2 and elem[0] == "not" and isVariable(elem[1]):
        return True
    if isVariable(elem):
        return True
    return False

def isVariable(elem):
    if type(elem) == str and len(elem) == 1 and elem.isalpha():
        return True
    return False

def isSymbol(elem):
    if elem == "and" or elem == "or" or elem == "iff" or elem == "implies" or elem == "not":
        return True
    return False

def convert(sentence):
    if len(sentence) == 0:
        return None;
    # ["A"]
    # ["not", "A"]
    if isLiteral(sentence):
        return sentence;
    # ["or", ]
    if sentence[0] == "or":
        # ["or", "A", ["not", "B"], "C" ...] single clause
        isSingleClause = True
        for index in range(1, len(sentence)):
            if not isLiteral(sentence[index]):
                 isSingleClause = False
        if isSingleClause:
            return sentence
        # ["or", "A", "B", ["and", "C", "D"], ["implies", "E", "F"], ["not", "G"], ...]
        if len(sentence) == 3:
            sentence = combine("or", sentence[1], sentence[2])
        else:
            newSentence = []
            newSentence.append("or")
            newSentence.append(combine("or", sentence[1], sentence[2]))
            for sent in sentence[3:]: newSentence.append(sent)
            sentence = newSentence
        return convert(sentence)
    if sentence[0] == "and":
        flag = True
        newSentence = []
        newSentence.append("and")
        for index in range(1, len(sentence)):
            if not isLiteral(sentence[index]):
                if sentence[index][0] == "and":
                    for sent in sentence[index][1:]: newSentence.append(sent)
                else:
                    isSingleClause = True
                    for index2 in range(1, len(sentence[index])):
                        if not isLiteral(sentence[index][index2]):
                            isSingleClause = False
                    if not isSingleClause:
                        if len(sentence[index]) == 3:
                            newSentence.append(combine("or",  sentence[index][1], sentence[index][2]))
                        else:
                            newSentence2 = []
                            newSentence2.append("or")
                            newSentence2.append(combine("or",  sentence[index][1], sentence[index][2]))
                            for sent in sentence[index][3:]: newSentence2.append(sent)
                            newSentence.append(newSentence2)
                        flag = False
                    else:
                        newSentence.append(sentence[index])
            else:
                newSentence.append(sentence[index])
        if flag == True:
            return newSentence
        return convert(newSentence)

def combine(symbol, sentence1, sentence2):
    if isLiteral(sentence1) and isLiteral(sentence2):
        return [symbol, sentence1, sentence2]
    if isLiteral(sentence1) and sentence2[0] == symbol:
        newSentence = sentence2[:]
        newSentence.append(sentence1)
        return newSentence
    if isLiteral(sentence2) and sentence1[0] == symbol:
        newSentence = sentence1[:]
        newSentence.append(sentence2)
        return newSentence
    if isLiteral(sentence1) and sentence2[0] != symbol:
        newSentence = []
        newSentence.append(sentence2[0])
        for index in range(1, len(sentence2)):
            newSentence.append(combine(symbol, sentence1, sentence2[index]))
        return newSentence
    if isLiteral(sentence2) and sentence1[0] != symbol:
        newSentence = []
        newSentence.append(sentence1[0])
        for index in range(1, len(sentence1)):
            newSentence.append(combine(symbol, sentence2, sentence1[index]))
        return newSentence
    if sentence1[0] == symbol and sentence2[0] == symbol:
        newSentence = []
        newSentence.append(symbol)
        for sent in sentence1[1:]: newSentence.append(sent)
        for sent in sentence2[1:]: newSentence.append(sent)
        return newSentence
    if sentence1[0] == symbol and sentence2[0] != symbol:
        newSentence = []
        newSentence.append(sentence2[0])
        for sent in sentence2[1:]: newSentence.append(combine(symbol, sentence1, sent))
        return newSentence
    if sentence2[0] == symbol and sentence1[0] != symbol:
        newSentence = []
        newSentence.append(sentence1[0])
        for sent in sentence1[1:]: newSentence.append(combine(symbol, sentence2, sent))
        return newSentence
    newSentence = []
    newSentence.append(sentence1[0])
    for sent in sentence2[1:]: newSentence.append(combine(symbol, sentence1, sent))
    return newSentence
 

def removeNotAndImply(sentence):
    if isLiteral(sentence):
        return sentence
    if sentence[0] == "not":
        if sentence[1][0] == "and" or sentence[1][0] == "or":
            newSentence = []
            if sentence[1][0] == "and":
                newSentence.append("or")
            else:
                newSentence.append("and")
            for index in range(1, len(sentence[1])):
                newSentence.append(["not", sentence[1][index]])
            return removeNotAndImply(newSentence)
        if sentence[1][0] == "not":
            if isVariable(sentence[1][1]):
                return sentence[1][1]
            else:
                return removeNotAndImply(sentence[1][1])
        if sentence[1][0] == "implies" or sentence[1][0] == "iff":
            newSentence = removeNotAndImply(sentence[1])
            newSentence = ["not", newSentence]
            return removeNotAndImply(newSentence)
    if sentence[0] == "and" or sentence[0] == "or":
        for index in range(1, len(sentence)):
            sentence[index] = removeNotAndImply(sentence[index])
        return sentence
    if sentence[0] == "implies":
        newSentence = []
        newSentence.append("or")
        newSentence.append(["not", sentence[1]])
        newSentence.append(sentence[2])
        return removeNotAndImply(newSentence)
    if sentence[0] == "iff":
        newSentence = []
        newSentence.append("and")
        newSentence.append(["implies", sentence[1], sentence[2]])
        newSentence.append(["implies", sentence[2], sentence[1]])
        return removeNotAndImply(newSentence)

def removeDup(sentence):
    newSentence = []
    if type(sentence) == list:
        for sent in sentence:
            if isSymbol(sent):
                newSentence.append(sent)
            elif not isLiteral(sent):
                sent = removeDup(sent)
                sent2 = []
                sent2.append(sent[0])
		sent2 += sorted(sent[1:])
                if sent2 not in newSentence:
                    newSentence.append(sent2)
            else:
                if sent not in newSentence:
                    newSentence.append(sent)
        if len(newSentence) == 2 and newSentence[0] != "not":
            newSentence = newSentence[1]
        return newSentence
    else:
        return sentence
            


def main():
    if len(sys.argv) != 3:
        print "Wrong arg num"
        return False
    if sys.argv[1] != "-i":
        print "Wrong arg"
        return False
    filename = sys.argv[2]
    result = open("sentences_CNF.txt", "wr")
    f = open(filename, "r")
    n = eval(f.readline())
    #result.write(str(n)+"\n")
    for i in range(n):
        l = eval(f.readline())
        l = removeNotAndImply(l)
        l = convert(l)
        l = removeDup(l)
        print l
        if isVariable(l):
            l = "'"+l+"'"
        result.write(str(l)+"\n")

main()

